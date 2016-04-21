#define _GNU_SOURCE

#include "common.h"

#include "MaxSLiCInterface.h"

extern max_file_t *TcpForwarding_init();
static struct in_addr netmask;
static struct in_addr dfe_top_ip;
static struct in_addr dfe_bot_ip;
static struct in_addr multicast_ip;
static struct in_addr *remote_ips;

static max_file_t *maxfile;
static max_engine_t *engine;

#define PACKED __attribute__((packed))

struct PACKED decision_rom {
	uint8_t should_forward;
	uint8_t consumer_id;
	uint64_t : 48;
};

struct PACKED event {
	uint64_t event_count;
	uint8_t  message_type;
	uint8_t  forward_decision;
	uint8_t  consumer_id;
	uint64_t : 40;
};

static void init_decision(size_t numConsmers);
static void init_consumers(size_t num_consumers);
static void init_multicast_feed();
static void events_loop(size_t num_consumers);

static max_tcp_socket_t **consumer_sockets;

uint16_t get_consumer_port(size_t consumer_index) {
	return CONSUMER_DST_PORT + consumer_index;
}

int main(int argc, char *argv[])
{
	static int num_mandatory_args = 4;
	if(argc < (num_mandatory_args+1)) {
		printf("Usage: %s <Top IP> <Bot IP> <multicast_ip> <consumer_ip1> [consumer_ip2 consumer_ip3 ...]\n", argv[0]);
		return 1;
	}

	size_t num_consumers = argc - num_mandatory_args;

	inet_aton("255.255.255.0", &netmask);
	printf("Local DFE Top @ %s\n", argv[1]);
	inet_aton(argv[1], &dfe_top_ip);
	printf("Local DFE Bot @ %s\n", argv[2]);
	inet_aton(argv[2], &dfe_bot_ip);
	printf("Multicast Feed @ %s:%d\n", argv[3], MULTICAST_PORT);
	inet_aton(argv[3], &multicast_ip);

	remote_ips = malloc(num_consumers * sizeof(struct in_addr));
	for (size_t i=0; i < num_consumers; i++) {
		printf("Consumer %zd @ %s:%d\n", i, argv[num_mandatory_args + i], get_consumer_port(i));
		inet_aton(argv[num_mandatory_args + i], &remote_ips[i]);
	}


	maxfile = TcpForwarding_init();
	engine = max_load(maxfile, "*");


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_run(engine, action);

	consumer_sockets = malloc(sizeof(max_tcp_socket_t*) * num_consumers);

	init_decision(num_consumers);
	init_multicast_feed();
	init_consumers(num_consumers);

	printf("Ready, going in to events loop.\n");
	events_loop(num_consumers);


	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

static void events_loop(size_t num_consumers)
{
	void *event_buffer;
	size_t num_slots = 512;
	size_t slot_size = sizeof(struct event);

	if (posix_memalign(&event_buffer, 4096, num_slots * slot_size)) {
		err(1, "Couldn't allocate events buffer");
	}

	max_llstream_t *event_stream = max_llstream_setup(engine, "events", num_slots, slot_size, event_buffer);

	while (true) {
		void *event_slot;
		if (max_llstream_read(event_stream, 1, &event_slot) == 1) {
			struct event *the_event = event_slot;
			printf("Event [%lu] type 0x%02x --> ", the_event->event_count, the_event->message_type);
			if (the_event->forward_decision == 0) printf("DROPPED\n");
			else printf("Forwarded to consumer %d\n", the_event->consumer_id);
			max_llstream_read_discard(event_stream, 1);
		}
	}
}

static void init_decision(size_t numConsmers)
{
	max_actions_t *action = max_actions_init(maxfile, NULL);
	struct decision_rom d;
	uint64_t *v = (void *)&d;

	for (size_t msgType = 0; msgType < 256; msgType++) {
		d.consumer_id = msgType % numConsmers;
		d.should_forward = msgType % 7 != 0;
		max_set_mem_uint64t(action, "fwd", "decisionRom", msgType, *v);
	}
	max_run(engine, action);
}

static void init_consumers(size_t num_consumers)
{
	printf("Setting up %zd consumer sockets...\n", num_consumers);
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &dfe_bot_ip, &netmask);

	for (size_t i=0; i< num_consumers; i++) {
		consumer_sockets[i] = max_tcp_create_socket_with_number(engine, "Consumers", i);
	}

	struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
	for (size_t i=0; i< num_consumers; i++) {
		max_tcp_connect(consumer_sockets[i], &remote_ips[i], get_consumer_port(i));
		max_tcp_await_state(consumer_sockets[i], MAX_TCP_STATE_ESTABLISHED, &timeout);
	}
}

static void init_multicast_feed()
{
	printf("Setting up multicast feed socket...\n");
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &dfe_top_ip, &netmask);
	max_udp_socket_t *dfe_socket = max_udp_create_socket(engine, "UdpMulticastFeed");
	max_udp_bind_ip(dfe_socket, &multicast_ip, MULTICAST_PORT);
}


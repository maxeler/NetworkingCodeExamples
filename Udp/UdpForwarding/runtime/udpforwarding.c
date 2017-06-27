#define _GNU_SOURCE


#include "common.h"

#include "MaxSLiCInterface.h"
#include "MaxSLiCNetInterface.h"
#include <max_udp_fast_path.h>

extern max_file_t *UdpForwarding_init();
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
};

struct PACKED event {
	uint64_t event_count;
	uint8_t message_type;
	uint8_t forward_decision;
	uint8_t consumer_id;
	uint64_t : 40;
};

static void init_decision(size_t numConsmers);
static void init_consumers(max_file_t *maxfile, max_engine_t * engine, size_t numConsumers);
static void init_multicast_feed(max_file_t *maxfile, max_engine_t * engine);
static void events_loop();

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


	maxfile = UdpForwarding_init();
	engine = max_load(maxfile, "*");

	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init_explicit(maxfile);
	max_run(engine, action);
	max_actions_free(action);
	max_reset_engine(engine);

	init_decision(num_consumers);
	init_multicast_feed(maxfile, engine);
	init_consumers(maxfile, engine, num_consumers);

	printf("Ready, going in to events loop.\n");
	events_loop();


	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

static void events_loop()
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
		d.should_forward = msgType % 2 == 0;
		max_set_mem_uint64t(action, "fwd", "decisionRom", msgType, *v);
	}
	max_run(engine, action);
}

static void init_consumers(max_file_t *maxfile, max_engine_t * engine, size_t numConsumers)
{
	printf("Setting up %zd consumer sockets...\n", numConsumers);
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &dfe_bot_ip, &netmask);

	max_udpfp_unitx_t *consumer_fp = NULL;
	if (max_udpfp_unitx_init(maxfile, engine, "Consumers", &consumer_fp)) {
		printf("fail to initialize consumer_fp\n");
		exit(1);
	}

	max_udpfp_error_t err;
	for (size_t i = 0; i < numConsumers; i++) {
		if ((err = max_udpfp_unitx_open(consumer_fp, i, CONSUMER_SRC_PORT + i, &remote_ips[i], get_consumer_port(i), 0))) {
			printf("fail to open consumer_fp for socket #%d, error msg: %s\n", i, max_udpfp_error_string(err));
			exit(1);
		}
	}
}

static void init_multicast_feed(max_file_t *maxfile, max_engine_t * engine)
{
	printf("Setting up multicast feed socket...\n");
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &dfe_top_ip, &netmask);
	max_udpfp_multirx_t *udpfp = NULL;
	if (max_udpfp_multirx_init(maxfile, engine, "UdpMulticastFeed", &udpfp)) {
		printf("init\n");
		exit(1);
	}

	if (max_udpfp_multirx_open(udpfp, 0, &multicast_ip, MULTICAST_PORT)) {
		printf("open\n");
		exit(1);
	}

	max_ip_multicast_join_group(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &multicast_ip);
}


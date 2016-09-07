#define _GNU_SOURCE


#include "common.h"
#include "MaxSLiCInterface.h"
#include "MaxSLiCNetInterface.h"

#define PACKED __attribute__((packed))

struct PACKED summary_s {
	uint64_t frame_id;
	uint32_t frame_size;
	uint32_t : 32; // padding
};

extern max_file_t *UdpLogging_init();
static void init_multicast_feed(max_engine_t * engine);
static void events_loop(max_engine_t * engine);
static struct in_addr netmask;
static struct in_addr dfe_top_ip;
static struct in_addr multicast_ip;

int main(int argc, char *argv[]) 
{
	static int num_mandatory_args = 2;
	if(argc < (num_mandatory_args+1)) {
		printf("Usage: %s <Top IP>  <multicast_ip>\n", argv[0]);
		return 1;
	}
	inet_aton("255.255.255.0", &netmask);
	printf("Local DFE Top @ %s\n", argv[1]);
	inet_aton(argv[1], &dfe_top_ip);
	printf("Multicast Feed @ %s:%d\n", argv[2], MULTICAST_PORT);
	inet_aton(argv[2], &multicast_ip);

	max_file_t *maxfile = UdpLogging_init();
	max_engine_t * engine = max_load(maxfile, "*");


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_run(engine, action);


	init_multicast_feed(engine);
	events_loop(engine);

	



	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

static void events_loop(max_engine_t * engine)
{
	void *summary_buffer;
	size_t num_slots = 512;
	size_t slot_size = sizeof(struct summary_s);

	if (posix_memalign(&summary_buffer, 4096, num_slots * slot_size)) {
		err(1, "Couldn't allocate events buffer");
	}

	max_llstream_t *summary_stream = max_llstream_setup(engine, "summary", num_slots, slot_size, summary_buffer);
	printf("Waiting for events...\n");

	while (true) {
		void *summary_slot;
		if (max_llstream_read(summary_stream, 1, &summary_slot) == 1) {
			struct summary_s *the_summary = summary_slot;
			printf("Software got frame [%lu] size %d\n", the_summary->frame_id, the_summary->frame_size);
			max_llstream_read_discard(summary_stream, 1);
		}
	}
}

static void init_multicast_feed(max_engine_t * engine)
{
	printf("Setting up multicast feed socket...\n");
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &dfe_top_ip, &netmask);
	max_udp_socket_t *dfe_socket = max_udp_create_socket(engine, "UdpMulticastFeed");
	max_udp_bind_ip(dfe_socket, &multicast_ip, MULTICAST_PORT);
}


#define _GNU_SOURCE

#include <errno.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <MaxSLiCInterface.h>
#include <MaxSLiCNetInterface.h>
#include <MaxTCP.h>

extern max_file_t *TcpFPOffload_init();
static int parse_args(int argc, char *argv[]);
static void init_sockets();
static void connect_remotes();
static void echo_loop();

static struct in_addr netmask;
static struct in_addr dfe_top_ip;
static struct in_addr *remote_ips;
static size_t num_remotes = 0;

static max_file_t *maxfile;
static max_engine_t *engine;
static max_tcp_t *max_tcp;


#define REMOTE_PORT_BASE 10000
static uint16_t get_remote_port(size_t remote_index) {
	return REMOTE_PORT_BASE + remote_index;
}

int main(int argc, char *argv[])
{
	if (parse_args(argc, argv)) return -1;

	maxfile = TcpFPOffload_init();
	engine = max_load(maxfile, "*");

	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_run(engine, action);

	init_sockets();
	connect_remotes();

	printf("Sockets connected. Going in to events loop.\n");

	echo_loop();



	max_fast_tcp_destroy(max_tcp);
	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

static void echo_loop()
{
	/*
	 * Setup framed streams
	 */
	size_t buffer_size = 4096 * 512;
	void *rx_buffer, *tx_buffer;
	if (posix_memalign(&rx_buffer, 4096, buffer_size)) {
		err(1, "Couldn't allocate rx_buffer %zd bytes", buffer_size);
	}

	if (posix_memalign(&tx_buffer, 4096, buffer_size)) {
		err(1, "Couldn't allocate tx_buffer %zd bytes", buffer_size);
	}

	max_framed_stream_t *rx_stream = max_framed_stream_setup(engine, "rx", rx_buffer, buffer_size, -1);
	max_framed_stream_t *tx_stream = max_framed_stream_setup(engine, "tx", rx_buffer, buffer_size, 2048);

	/*
	 * Everything we receive we will echo back.
	 * If a connection drops, we will terminate.
	 */

	while (true) {
		void *rx_frame;
		void *tx_frame;
		size_t rx_frame_size;
		if (max_framed_stream_read(rx_stream, 1, &rx_frame, &rx_frame_size) == 1) {
			/*
			 * Frame header contains the socket ID
			 */
			uint16_t *data = rx_frame;
			printf("Socket[%u]: Echoing back %zd bytes\n", *(uint16_t *)data, rx_frame_size);

			/*
			 * We're going to leave the header in place since we're echoing the data back to the same socket.
			 */
			while (max_framed_stream_write_acquire(tx_stream, 1, &tx_frame) != 1);
			memcpy(tx_frame, rx_frame, rx_frame_size);
			max_framed_stream_write(tx_stream, 1, &rx_frame_size);
			max_framed_stream_discard(rx_stream, 1);
		}

		for (size_t i=0; i < num_remotes; i++) {
			max_tcp_socket_state_t state;
			max_fast_tcp_get_socket_state(max_tcp, i, &state);
			if (state != MAXTCP_SOCKET_STATE_ESTABLISHED) {
				printf("Socket %zd is not longer established, terminating.", i);
				return;
			}
		}
	}
}

static void init_sockets()
{
	max_net_connection_t connection = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1;
	max_ip_config(engine, connection, &dfe_top_ip, &netmask);
	struct in_addr gw = { .s_addr = 0 };
	max_ip_route_set_default_gw(engine, connection, &gw);
	max_tcp = max_fast_tcp_init(maxfile, engine, "tcp");
	if(max_tcp == NULL) {
		err(1, "max_fast_tcp_init failed");
	}
}

static void connect_remotes()
{
	struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
	for (size_t i=0; i< num_remotes; i++) {
		max_tcp_error_t e = max_fast_tcp_connect(max_tcp, i, &remote_ips[i], get_remote_port(i));
		if (e) {
			err(1, "max_fast_tcp_connect returned error: %s",
					max_fast_tcp_get_error_message(e));
		}
		max_fast_tcp_wait_for_socket_state(max_tcp, i, MAXTCP_SOCKET_STATE_ESTABLISHED, &timeout);
	}
}

static int parse_args(int argc, char *argv[])
{
	static int num_mandatory_args = 2;

	if(argc < (num_mandatory_args+1)) {
		printf("Usage: %s <Top IP> <remote_ip1> [remote_ip2 remote_ip3 ...]\n", argv[0]);
		return 1;
	}

	inet_aton("255.255.255.0", &netmask);
	printf("Local DFE Top @ %s\n", argv[1]);
	inet_aton(argv[1], &dfe_top_ip);

	num_remotes = argc - num_mandatory_args;

	remote_ips = malloc(num_remotes * sizeof(struct in_addr));
	for (size_t i=0; i < num_remotes; i++) {
		printf("Remote %zd @ %s:%d\n", i, argv[num_mandatory_args + i], get_remote_port(i));
		inet_aton(argv[num_mandatory_args + i], &remote_ips[i]);
	}

	return 0;
}

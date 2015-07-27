#define _GNU_SOURCE

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <MaxSLiCInterface.h>

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Usage: $0 dfe_ip remote_ip\n");
		return 1;
	}

	struct in_addr dfe_ip;
	inet_aton(argv[1], &dfe_ip);
	struct in_addr remote_ip;
	inet_aton(argv[2], &remote_ip);
	struct in_addr netmask;
	inet_aton("255.255.255.0", &netmask);
	const int in_port = 2000;
	const int out_port = 2000;

//	struct in_addr mcastaddr;
//	inet_aton("224.0.0.1", &mcastaddr);

	max_file_t *maxfile = SignExtWithPatternMatching_init();
	max_engine_t * engine = max_load(maxfile, "*");

	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *actions = max_actions_init(maxfile, NULL);

	max_run(engine, actions);
	max_actions_free(actions);


	void *buffer;
	size_t bufferSize = 4096 * 512;
	posix_memalign(&buffer, 4096, bufferSize);

	max_framed_stream_t *toCpu = max_framed_stream_setup(engine, "toCPU", buffer, bufferSize, -1);

	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &dfe_ip, &netmask);
	max_udp_socket_t *dfe_socket = max_udp_create_socket(engine, "udpTopPort1");
//	max_ip_multicast_join_group(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &mcastaddr);
//	max_udp_bind_ip(dfe_socket, &mcastaddr, in_port);
	max_udp_bind(dfe_socket, in_port);
	max_udp_connect(dfe_socket, &remote_ip, out_port);

	printf("Listening on %s in_port %d\n", argv[1], in_port);

	printf("Waiting for kernel response...\n"); fflush(stdout);

	void *f;
	size_t fsz;
	size_t numMessageRx = 0;
	while (1) {
		if (max_framed_stream_read(toCpu, 1, &f, &fsz) == 1) {
			numMessageRx++;

			printf("CPU: Got output frame %zd - size %zd bytes\n", numMessageRx, fsz);

			uint64_t *w = f;
			for (size_t i=0; i < 3; i++) {
				printf("Frame [%zd] Word[%zd]: 0x%lx\n", numMessageRx, i, w[i]);
			}


			max_framed_stream_discard(toCpu, 1);
		} else 	usleep(10);
	}

//	max_ip_multicast_leave_group(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &mcastaddr);
	max_udp_close(dfe_socket);
	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n"); fflush(stdout);
	return 0;
}

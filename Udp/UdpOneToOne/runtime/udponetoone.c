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
#include <MaxSLiCNetInterface.h>


#define IN_PORT 9910
#define OUT_PORT 9920


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

//	struct in_addr mcastaddr;
//	inet_aton("224.0.0.1", &mcastaddr);

	max_file_t *maxfile = UdpOneToOne_init();
	max_engine_t * engine = max_load(maxfile, "*");

	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *actions = max_actions_init(maxfile, NULL);

	max_run(engine, actions);
	max_actions_free(actions);


	void *toCpuBuffer, *fromCpuBuffer;
	size_t bufferSize = 4096 * 512;
	posix_memalign(&toCpuBuffer, 4096, bufferSize);
	posix_memalign(&fromCpuBuffer, 4096, bufferSize);

	max_framed_stream_t *toCpu = max_framed_stream_setup(engine, "toCpu", toCpuBuffer, bufferSize, -1);
	max_framed_stream_t *fromCpu = max_framed_stream_setup(engine, "fromCpu", fromCpuBuffer, bufferSize, 512);

	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &dfe_ip, &netmask);
	max_udp_socket_t *dfe_socket = max_udp_create_socket(engine, "udpTopPort1");
//	max_udp_bind_ip(dfe_socket, &mcastaddr, IN_PORT);
	max_udp_bind(dfe_socket, IN_PORT);
	max_udp_connect(dfe_socket, &remote_ip, OUT_PORT);

	printf("Listening on %s port %d\n", argv[1], IN_PORT); fflush(stdout);

	void *toCpuFrame, *fromCpuFrame;
	size_t fsz;
	size_t numMessages = 0;
	while (1) {
		while (max_framed_stream_write_acquire(fromCpu, 1, &fromCpuFrame) == 0);
		while (max_framed_stream_read(toCpu, 1, &toCpuFrame, &fsz) == 0);

		{
			numMessages++;

			printf("DFE: Got frame [%zd] - '%.*s' size %zd bytes\n", numMessages, (int)fsz, (char *)toCpuFrame, fsz);


			/*
			 * Send it back
			 */

			memcpy(fromCpuFrame, toCpuFrame, fsz);
			max_framed_stream_write(fromCpu, 1, &fsz);

			max_framed_stream_discard(toCpu, 1);
		}
	}

	max_udp_close(dfe_socket);
	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n"); fflush(stdout);
	return 0;
}

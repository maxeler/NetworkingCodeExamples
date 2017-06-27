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
#include <max_udp_fast_path.h>


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

	max_udpfp_error_t udp_err;
	max_udpfp_unirx_t *max_udpfp_rx;
	udp_err = max_udpfp_unirx_init(maxfile, engine, "udpRx", &max_udpfp_rx);
	if(udp_err) {
		fprintf(stderr, "Failed to initialize hardware UDP rx stream: %s\n",
				max_udpfp_error_string(udp_err));
		exit(1);
	}

	max_udpfp_unitx_t *max_udpfp_tx;
	udp_err = max_udpfp_unitx_init(maxfile, engine, "udpTx", &max_udpfp_tx);
	if(udp_err) {
		fprintf(stderr, "Failed to initialize hardware UDP tx stream: %s\n",
				max_udpfp_error_string(udp_err));
		exit(1);
	}

	int hw_rx_socket = 0;
	udp_err = max_udpfp_unirx_open(max_udpfp_rx, hw_rx_socket, IN_PORT, &remote_ip, OUT_PORT);
	if(udp_err) {
		fprintf(stderr, "Failed to open hardware UDP rx stream: %s\n",
				max_udpfp_error_string(udp_err));
		exit(1);
	}

	int hw_tx_socket = 0;
	udp_err = max_udpfp_unitx_open(max_udpfp_tx, hw_tx_socket, IN_PORT, &remote_ip, OUT_PORT, 0);
	if(udp_err) {
		fprintf(stderr, "Failed to open hardware UDP tx stream: %s\n",
				max_udpfp_error_string(udp_err));
		exit(1);
	}

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

	max_udpfp_unirx_close(max_udpfp_rx, hw_rx_socket);
	max_udpfp_unitx_close(max_udpfp_tx, hw_tx_socket);
	max_udpfp_unirx_destroy(max_udpfp_rx);
	max_udpfp_unitx_destroy(max_udpfp_tx);

	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n"); fflush(stdout);
	return 0;
}

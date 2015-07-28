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

#define PACKED __attribute__ ((__packed__))
#define NUM_MESSAGES_EXPECTED 7

typedef struct PACKED ef_data_s {
	uint64_t data : 48;
} ef_data_t;

typedef struct PACKED msg_format_s {
	uint16_t numExtraFields;
	uint32_t fieldA;
	uint8_t fieldB;
	uint32_t fieldC : 24;

	// Start ef_data_t
} msg_format_t;

typedef struct PACKED message_format_s {
	uint8_t version;
	uint8_t flags;
	uint16_t numMessages;

	// Start msg_format_t
} bnp_format_t;


static void *genData(size_t *sz) {
	void *buffer = malloc(4096);

	bnp_format_t *p = buffer;
	p->version = 1;
	p->flags = 0xFF;

	uint16_t numMessages = NUM_MESSAGES_EXPECTED;

	p->numMessages = numMessages;

	msg_format_t *msg = (void *)(p + 1);

	for (uint16_t m=0; m < numMessages; m++) {
		msg->fieldA = 0xAA;
		msg->fieldB = 0xBB;
		msg->fieldC = 0xCC;

		uint16_t numExtraFields = m+1;
		msg->numExtraFields = numExtraFields;

		uint64_t off = (void*)msg - buffer;
		printf("Msg %u, Num EF = %u --> Offset %lu, mod %lu\n", m, numExtraFields, off, off % 8);

		ef_data_t *ef = (void *)(msg + 1);

		for (size_t e=0; e < numExtraFields; e++) {
			ef[e].data = e;
		}

		ef += numExtraFields;
		msg = (msg_format_t *)ef;
	}

	*sz = (uint64_t)(msg + 1) - (uint64_t)buffer;

	return buffer;
}


static int create_cpu_udp_socket(struct in_addr *local_ip, struct in_addr *remote_ip, int port) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in cpu;
	memset(&cpu, 0, sizeof(cpu));
	cpu.sin_family = AF_INET;
	cpu.sin_port = htons(port);

	cpu.sin_addr = *local_ip;
	bind(sock, (struct sockaddr *)&cpu, sizeof(cpu));

	cpu.sin_addr = *remote_ip;
    connect(sock, (const struct sockaddr*) &cpu, sizeof(cpu));

	return sock;
}


void sendTestFrame(int sock) {
	size_t dataSz = 0;
	void *data = genData(&dataSz);


	send(sock, data, dataSz, 0);

}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Usage: $0 dfe_ip cpu_ip\n");
		return 1;
	}

	struct in_addr dfe_ip;
	inet_aton(argv[1], &dfe_ip);
	struct in_addr cpu_ip;
	inet_aton(argv[2], &cpu_ip);
	struct in_addr netmask;
	inet_aton("255.255.255.0", &netmask);
	const int port = 5007;

	max_file_t *maxfile = Tracker_init();
	max_engine_t * engine = max_load(maxfile, "*");


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *actions = max_actions_init(maxfile, NULL);
	char regName[32];
	for (int i=0; i < 1024; i++) {
		sprintf(regName, "filter_%d", i);
		if (i == 150) {
			max_set_uint64t(actions, "filteringKernel", regName, 0xCC /* a value to match... */);
		} else {
			max_set_uint64t(actions, "filteringKernel", regName, 0x4D1B /* or any value you want */);
		}
	}
	max_run(engine, actions);
	max_actions_free(actions);


	void *buffer;
	size_t bufferSize = 4096 * 512;
	posix_memalign(&buffer, 4096, bufferSize);

	max_framed_stream_t *toCpu = max_framed_stream_setup(engine, "toCPU", buffer, bufferSize, -1);

	/*
	 * This executable both creates a normal Linux UDP socket as well as a DFE UDP Socket.
	 * We then exchange data between the two.
	 */

	// DFE Socket
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &dfe_ip, &netmask);
	max_udp_socket_t *dfe_socket = max_udp_create_socket(engine, "udpTopPort1");
	max_udp_bind(dfe_socket, port);
	max_udp_connect(dfe_socket, &cpu_ip, port);


	// Linux Socket
	int cpu_socket = create_cpu_udp_socket(&cpu_ip, &dfe_ip, port);

	printf("Sending test frame...\n");
	sendTestFrame(cpu_socket);

	printf("Waiting for kernel response...\n"); fflush(stdout);

	void *f;
	size_t fsz;
	size_t numMessageRx = 0;
	uint8_t received_data[512];
	while (numMessageRx < NUM_MESSAGES_EXPECTED) {
		if (max_framed_stream_read(toCpu, 1, &f, &fsz) == 1) {
			printf("CPU: Got output frame - size %zd - NumMsg = %zd!\n", fsz, numMessageRx); // Frame size would be rounded up to the next 8 bytes.

			memcpy(received_data, f, fsz);
			numMessageRx++;
			max_framed_stream_discard(toCpu, 1);
		} else 	usleep(10);
	}

	max_udp_close(dfe_socket);
	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n"); fflush(stdout);
	return 0;
}

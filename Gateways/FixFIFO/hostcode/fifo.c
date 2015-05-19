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
#include <ctype.h>

#include <MaxSLiCInterface.h>

#define MAX_SOCKETS 1024
#define MAX_OUTBOUND_FRAME_SIZE 2000
#define MIN(x, y) ((x) < (y) ? (x) : (y))
static max_file_t *maxfile;
static max_engine_t * engine;

enum SocketState {
		SocketStateClosed = 0,
		SocketStateListen = 1,
		SocketStateSynSent = 2,
		SocketStateSynRcvd = 3,
		SocketStateEstablished = 4,
		SocketStateCloseWait = 5,
		SocketStateFinWait1 = 6,
		SocketStateClosing = 7,
		SocketStateLastAck = 8,
		SocketStateFinWait2 = 9,
		SocketStateTimeWait = 10,
		SocketStateClearTcp = 11,
		SocketStateClosedDataPending = 13
};

struct InboundHeaderType_s {
	uint16_t  socket;
	uint8_t   errorCode : 3;
	uint8_t   connectionState : 4;
	uint8_t   connectionStateValid : 1;
	uint8_t   currentState : 5;
	uint8_t   containsData : 1;
	uint8_t   protocolID : 1;
	uint8_t   padding : 1;
	int32_t   level : 18;
	uint16_t  padding14 : 14;
};

struct OutboundHeaderType_s {
	uint16_t socket_num;
	uint64_t : 48;
};

struct socket_context {
	uint16_t socket_id;
	uint16_t port;
	max_tcp_socket_t *theSocket;
	enum SocketState socketState;
};

static struct socket_context *sockets = NULL;

static struct {
	bool is_sim;
	max_net_connection_t eth_conn;
	uint16_t num_connections;

	uint32_t total_rx_window_size_bytes;
	uint32_t total_tx_window_size_bytes;
} constants;

enum FramerCodes {
	FramerCodeNoError = 0,
	FramerCodeHeaderCorrupt = 1,
	FramerCodePayloadError = 2,
	FramerCodeShutdownDrain = 3,
	FramerCodeBodyLengthTooBig = 4,
	FramerCodePayloadCutShort = 6,
	FramerCodePreviousErrors = 7
};


static char *framerCodeToString(enum FramerCodes code) {
	switch (code) {
		case FramerCodeNoError:				return "NoError";
		case FramerCodeHeaderCorrupt: 		return "HeaderCorrupt";
		case FramerCodePayloadError: 		return "PayloadError";
		case FramerCodeShutdownDrain: 		return "ShutdownDrain";
		case FramerCodeBodyLengthTooBig:	return "BodyLengthTooBig";
		case FramerCodePayloadCutShort: 	return "PayloadCutShort";
		case FramerCodePreviousErrors: 		return "PreviousErrors";
	}

	return "Unknown";
}

static char *socketStateToString(enum SocketState state) {
	switch (state) {
		case SocketStateClosed:				return "Closed";
		case SocketStateListen:				return "Listen";
		case SocketStateSynSent: 			return "SynSent";
		case SocketStateSynRcvd: 			return "SynRcvd";
		case SocketStateEstablished: 		return "Established";
		case SocketStateCloseWait: 			return "CloseWait";
		case SocketStateFinWait1: 			return "FinWait1";
		case SocketStateClosing: 			return "Closing";
		case SocketStateLastAck: 			return "LastAck";
		case SocketStateFinWait2: 			return "FinWait2";
		case SocketStateTimeWait: 			return "TimeWait";
		case SocketStateClearTcp: 			return "ClearTcp";
		case SocketStateClosedDataPending: 	return "ClosedDataPending";
	}

	return "Unknown";
}

static void maxfile_read_constants() {
	constants.is_sim = max_get_constant_uint64t(maxfile, "IS_SIMULATION") == 1;

	const char *ethConnString = max_get_constant_string(maxfile, "ethConnection");

	if (strcmp(ethConnString, "QSFP_TOP_10G_PORT1") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1;
	else if (strcmp(ethConnString, "QSFP_TOP_10G_PORT2") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT2;
	else if (strcmp(ethConnString, "QSFP_TOP_10G_PORT3") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT3;
	else if (strcmp(ethConnString, "QSFP_TOP_10G_PORT4") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT4;
	else if (strcmp(ethConnString, "QSFP_MID_10G_PORT1") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_MID_10G_PORT1;
	else if (strcmp(ethConnString, "QSFP_MID_10G_PORT2") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_MID_10G_PORT2;
	else if (strcmp(ethConnString, "QSFP_MID_10G_PORT3") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_MID_10G_PORT3;
	else if (strcmp(ethConnString, "QSFP_MID_10G_PORT4") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_MID_10G_PORT4;
	else if (strcmp(ethConnString, "QSFP_BOT_10G_PORT1") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1;
	else if (strcmp(ethConnString, "QSFP_BOT_10G_PORT2") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT2;
	else if (strcmp(ethConnString, "QSFP_BOT_10G_PORT3") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT3;
	else if (strcmp(ethConnString, "QSFP_BOT_10G_PORT4") == 0)
		constants.eth_conn = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT4;
	else {
		printf("ethConnection in maxfile (\"%s\") was not recognised.\n", ethConnString);
		exit(1);
	}

	constants.total_rx_window_size_bytes = max_get_constant_uint64t(maxfile, "rxWindowSizeKB") * 1024;
	constants.total_tx_window_size_bytes = max_get_constant_uint64t(maxfile, "txWindowSizeKB") * 1024;
}

static void init_tcp(char *dfe_ip, uint16_t num_sockets) {
	struct in_addr netmask;
	inet_aton("255.255.255.0", &netmask);
	struct in_addr ip;
	inet_aton(dfe_ip, &ip);

	if (constants.is_sim) {
		if (num_sockets > 64) {
			printf("Max sockets supported in simulation is 64!\n");
			exit(1);
		}
	}

	printf("Configuring DFE with IP %s and %u sockets\n", dfe_ip, num_sockets);

	max_ip_config(engine, constants.eth_conn, &ip, &netmask);

	if (num_sockets > MAX_SOCKETS) {
		printf("Maximum number of sockets exceeded. Max supported %u\n", MAX_SOCKETS);
		exit(1);
	}

	sockets = calloc(num_sockets, sizeof(struct socket_context));

	for (uint16_t i=0; i < num_sockets; i++) {
		sockets[i].socket_id = i;
		sockets[i].port = 1000+i;
		sockets[i].theSocket = max_tcp_create_socket_with_number(engine, "tcpStream", i);
		sockets[i].socketState = SocketStateClosed;
	}
}

static void Listen(uint16_t socket_num, uint16_t port) {
	if (sockets[socket_num].socketState == SocketStateClosed) {
//		max_actions_t *action = max_actions_init(maxfile, NULL);
//		max_enable_partial_memory(action);
//		max_set_mem_uint64t(action, "gateway", "protocolSelect", socket_num, 0 /* protocol */);
//		max_run(engine, action);

		sockets[socket_num].port = port;
		max_tcp_listen(sockets[socket_num].theSocket, port);

	} else {
		printf("Cannot listen on socket %u, socket state is '%s' but should be Closed.\n", socket_num, socketStateToString(sockets[socket_num].socketState));
		exit(1);
	}
}

static void CloseGraceful(uint16_t socket_num) {
	max_tcp_close_advanced(sockets[socket_num].theSocket, MAX_TCP_CLOSE_GRACEFUL);
}

static void CloseReset(uint16_t socket_num) {
	max_tcp_close_advanced(sockets[socket_num].theSocket, MAX_TCP_CLOSE_ABORT_RESET);
}

//static void CloseKill(uint16_t socket_num) {
//	max_tcp_close_advanced(sockets[socket_num].theSocket, MAX_TCP_CLOSE_ABORT_NO_RESET);
//}


static void Close(uint16_t socket_num) {
	if (sockets[socket_num].socketState == SocketStateEstablished) {
		CloseGraceful(socket_num);
	} else if (sockets[socket_num].socketState == SocketStateClosed) {
		// Socket already closed, do nothing
	} else {
		// Socket in some other state
		CloseReset(socket_num);
	}
}

static void printFix(char *fix, size_t size) {
	for (size_t i=0; i < size; i++) {
		if (fix[i] != 1 && !isprint(fix[i])) { printf("\033[40;32;1m.\033[0m"); }
		putchar(fix[i] == 1 ? '|' : fix[i]);
	}
	putchar('\n');
}


static void transition(uint16_t socket_num, enum SocketState toState) {
	printf("socket[%u] state transition: %s -> %s\n", socket_num, socketStateToString(sockets[socket_num].socketState), socketStateToString(toState));
	sockets[socket_num].socketState = toState;

	/*
	 * Take some action based on the new state
	 */
	switch (toState) {
	case SocketStateCloseWait:
		CloseGraceful(socket_num);
		break;
	case SocketStateClosedDataPending:
		CloseGraceful(socket_num);
		break;
	case SocketStateClosed:
		// Re-open
		Listen(socket_num, sockets[socket_num].port);
		break;
	default:
		break;
	}
}

static void Send(max_framed_stream_t *outboundStream, uint16_t socketNum, void *payload, size_t payloadSize) {
	uint8_t *pb = payload;
	size_t pos = 0;

	while (pos < payloadSize) {
		size_t sendNow = MIN(payloadSize - pos, MAX_OUTBOUND_FRAME_SIZE - sizeof(struct OutboundHeaderType_s));

		void *vSendFrame;
		while (max_framed_stream_write_acquire(outboundStream, 1, &vSendFrame) == 0);

		struct OutboundHeaderType_s *sendFrame = vSendFrame;
		sendFrame->socket_num = socketNum;

		memcpy(sendFrame + 1, pb + pos, sendNow);

		size_t actualSize = sendNow + sizeof(struct OutboundHeaderType_s);
		max_framed_stream_write(outboundStream, 1, &actualSize);


		pos += sendNow;
	}
}

static void process(max_framed_stream_t *toCpuStream, max_framed_stream_t *outboundStream) {
	void *frame;
	size_t frame_size;

	static	size_t frameCount = 0;
	while (max_framed_stream_read(toCpuStream, 1, &frame, &frame_size) == 1) {
		struct InboundHeaderType_s *header = frame;
		frameCount++;


		uint16_t socket_num = header->socket;
		printf("socket[%u] Event - frame size %zd\n", socket_num, frame_size);

		if (header->connectionStateValid) {
			transition(socket_num, header->connectionState);
		}

		if (header->errorCode != FramerCodeNoError) {
			printf("socket[%u] Framer code: %d --> %s\n", socket_num, header->errorCode, framerCodeToString(header->errorCode));
		}

		if (header->containsData) {
			printf("socket[%u] Contains data\n", socket_num);
			void *payload = header + 1;
			if (frame_size > sizeof(struct InboundHeaderType_s)) {
				size_t payloadSize = frame_size - sizeof(struct InboundHeaderType_s);

				printf("socket[%u] Got payload: ", socket_num);
				printFix(payload, payloadSize);

				/*
				 * Echo back
				 */
				Send(outboundStream, socket_num, payload, payloadSize);
			}
		}

		max_framed_stream_discard(toCpuStream, 1);
		fflush(stdout);
	}
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Usage: $0 dfe_ip num_sockets\n");
		return 1;
	}

	maxfile = FIFOGateway_init();
	engine = max_load(maxfile, "*");
	maxfile_read_constants();

	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	char *dfe_ip = argv[1];
	uint16_t num_sockets = (uint16_t)atoi(argv[2]);

	init_tcp(dfe_ip, num_sockets);

	max_actions_t *actions = max_actions_init(maxfile, NULL);

	max_run(engine, actions);
	max_actions_free(actions);


	void *toCpuBuffer, *outboundBuffer;
	size_t bufferSize = 4096 * 512;
	posix_memalign(&toCpuBuffer, 4096, bufferSize);
	posix_memalign(&outboundBuffer, 4096, bufferSize);

	max_framed_stream_t *toCpu = max_framed_stream_setup(engine, "toCPU", toCpuBuffer, bufferSize, -1);
	max_framed_stream_t *outboundStream = max_framed_stream_setup(engine, "fromCPU", outboundBuffer, bufferSize, MAX_OUTBOUND_FRAME_SIZE);

	if (constants.is_sim) {
		for (uint16_t i=0; i < num_sockets; i++) {
			Listen(i, 1000 + i);
		}
	}

	while (true) {
		process(toCpu, outboundStream);
		usleep(1);
	}

	for (uint16_t i=0; i < num_sockets; i++) {
		Close(i);
	}


	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n"); fflush(stdout);
	return 0;
}

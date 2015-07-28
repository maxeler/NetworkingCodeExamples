#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>


static struct sockaddr_in sockaddr;

static int create_socket(struct in_addr *local_ip, int port) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);

	sockaddr.sin_addr = *local_ip;
	if (bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		printf("Bind failed: %s\n", strerror(errno));
		exit(1);
	}

//	sockaddr.sin_addr = *remote_ip;
//    connect(sock, (const struct sockaddr*) &sockaddr, sizeof(sockaddr));

	return sock;
}


static void recv_frames(int sock) {
	uint8_t *frame_buffer = malloc(2048);

	size_t frameCount = 0;
	while (true) {
		socklen_t socklen = sizeof(struct sockaddr_in);
		int now = recvfrom(sock, frame_buffer, 2048, 0, (struct sockaddr *)&sockaddr, &socklen);
		if (now < 0) {
			printf("recvfrom error: %s\n", strerror(errno));
			exit(1);
		}

		frame_buffer[now] = 0;
		frameCount++;
		printf("Got frame %zd: '%s'\n", frameCount, frame_buffer);
	}
}


int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Syntax: %s <local ip address>\n", argv[0]);
		return 1;
	}

	struct in_addr local_ip;
	int port = 7653;

	printf("Local IP: '%s', port %d\n", argv[1], port);

	inet_aton(argv[1], &local_ip);


	int mySocket = create_socket(&local_ip, port);

	recv_frames(mySocket);

	printf("Receiver finished\n");
	return 0;
}


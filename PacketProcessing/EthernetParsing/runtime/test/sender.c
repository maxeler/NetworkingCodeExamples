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



static int create_socket(struct in_addr *remote_ip, int port) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);

//	sockaddr.sin_addr = *local_ip;
//	bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

	sockaddr.sin_addr = *remote_ip;
    connect(sock, (const struct sockaddr*) &sockaddr, sizeof(sockaddr));

	return sock;
}


static void send_frames(int sock) {
	void *frame_buffer = malloc(2048);

	for (int j = 0; j < 100; j++) {
		uint8_t *pb = frame_buffer;
		int pos = 0;
		int size = sprintf(frame_buffer, "This is frame number %d\n", j);

		while (pos < size) {
			int now = send(sock, pb + pos, size - pos, 0);
			if (now > 0) {
				pos += now;
			} else {
				printf("Can't send frame %d: %s\n", j, strerror(errno));
				exit(1);
			}
		}
		printf("Sent frame %d\n", j);
		usleep(1000 * 100);
	}
}


int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Syntax: %s <remote ip address>\n", argv[0]);
		return 1;
	}

	struct in_addr remote_ip;
	int port = 7653;

	printf("Remote IP: '%s', port %d\n", argv[1], port);
	inet_aton(argv[1], &remote_ip);

	int mySocket = create_socket(&remote_ip, port);

	send_frames(mySocket);

	printf("Sender finished\n");
	return 0;
}


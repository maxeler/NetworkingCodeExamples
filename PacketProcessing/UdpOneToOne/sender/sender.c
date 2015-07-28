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

#define OUT_PORT 9910
#define IN_PORT 9920

static int create_socket(struct in_addr *remote_ip) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(IN_PORT);

//	sockaddr.sin_addr = *local_ip;
	if (bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != 0) {
		printf("Bind failed: %s\n", strerror(errno));
		exit(1);
	}

	sockaddr.sin_port = htons(OUT_PORT);
	sockaddr.sin_addr = *remote_ip;
    connect(sock, (const struct sockaddr*) &sockaddr, sizeof(sockaddr));

	return sock;
}

static void send_frames(int sock) {
	size_t max_buffer_size = 512;
	void *txBuffer = malloc(max_buffer_size);
	void *rxBuffer = malloc(max_buffer_size);

	int realSize = snprintf(txBuffer, max_buffer_size, "This is a test!\n");
	
	
	send(sock, txBuffer, realSize, MSG_WAITALL);
	printf("Sent buffer...");
	ssize_t rxSize = 0;
	while ((rxSize = recv(sock, rxBuffer, max_buffer_size, 0)) == 0) usleep(1);
	if (rxSize > 0) {
		printf("Got response '%.*s'\n", (int)rxSize, (char *)rxBuffer);
	} else {
		printf("Error waiting for response: %s\n", strerror(errno));
	}
}


int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	struct in_addr remote_ip;
	inet_aton("172.16.50.1", &remote_ip);

	int mySocket = create_socket(&remote_ip);

	send_frames(mySocket);

	printf("Sender finished\n");
	getchar();
}


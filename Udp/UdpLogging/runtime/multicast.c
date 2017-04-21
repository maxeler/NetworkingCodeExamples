/*
 * multicast.c
 *
 *  Created on: 18 Apr 2016
 *      Author: itay
 */

#include "common.h"

static void bind_source_address(int sock, char *source_addr);

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("Usage: %s <source ip> <multicast ip>\n", argv[0]);
		return 1;
	}

	printf("Generating Mutlicast feed: %s:%d\n", argv[1], MULTICAST_PORT);

	struct sockaddr_in addr;
	int fd, cnt;
	struct ip_mreq mreq;
	char *message = malloc(60);

	/* create what looks like an ordinary UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	bind_source_address(fd, argv[1]);

	/* set up destination address */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[2]);
	addr.sin_port = htons(MULTICAST_PORT);



	/* now just sendto() our destination! */
	int max_len = 60;
	for (size_t i=58; i < max_len; i++) {
		memset(message, 0xAA, i);
		printf("Sending messages - size %zd\n", i);
		uint64_t *number = (void *)(message + 50);
		*number = 0x1122334455667788UL;

		if (sendto(fd, message, i, 0, (struct sockaddr *) &addr,
				sizeof(addr)) < 0) {
			perror("sendto");
			exit(1);
		}
		sleep(1);
	}

	return 0;
}

static void bind_source_address(int sock, char *source_addr)
{
	printf("Binding to %s:%d\n", source_addr, MULTICAST_PORT);
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(MULTICAST_PORT);
	sockaddr.sin_addr.s_addr = inet_addr(source_addr);

	if (bind(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
		err(1, "Bind on source multicast %s:%d", source_addr, MULTICAST_PORT);
	}
}

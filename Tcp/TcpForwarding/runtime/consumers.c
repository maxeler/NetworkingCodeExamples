/*
 * consumers.c
 *
 *  Created on: 15 Apr 2016
 *      Author: itay
 */

#include "common.h"
#include <sys/epoll.h>

#define MAX_EVENTS 64

static int create_socket(size_t consumer_index);
static void event_loop(int *consumers, size_t num_consumers);
static void read_consumer(int *consumers, size_t consumer_index);

static struct in_addr netmask;

uint16_t get_consumer_port(size_t consumer_index)
{
	return CONSUMER_DST_PORT + consumer_index;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <num consumers>\n", argv[0]);
		return 1;
	}

	size_t num_consumers = atoi(argv[1]);
	int *sockets = malloc(sizeof(int) * num_consumers);

	inet_aton("255.255.255.0", &netmask);
	for (size_t i = 0; i < num_consumers; i++) {
		printf("Consumer %zd - listen port %d\n", i, get_consumer_port(i));
		sockets[i] = create_socket(i);
	}

	event_loop(sockets, num_consumers);
}

static void event_loop(int *consumers, size_t num_consumers)
{
	struct epoll_event ev, events[MAX_EVENTS];
	int listen_sock, conn_sock, nfds, epollfd;

	epollfd = epoll_create(MAX_EVENTS);
	if (epollfd == -1) {
		perror("epoll_create");
		exit(EXIT_FAILURE);
	}

	for (size_t i=0; i < num_consumers; i++) {
		ev.events = EPOLLIN;
		ev.data.u32 = i;
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, consumers[i], &ev) == -1) {
			perror("epoll_ctl: listen_sock");
			exit(EXIT_FAILURE);
		}
	}

	for (;;) {
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			perror("epoll_pwait");
			exit(EXIT_FAILURE);
		}

		for (int n = 0; n < nfds; ++n) {
			/*
			 * Drain socket
			 */
			read_consumer(consumers, events[n].data.u32);
		}
	}
}

static void read_consumer(int *consumers, size_t consumer_index)
{
	static size_t max_buffer_size = 2048;
	static void *rx_buffer = NULL;

	if (rx_buffer == NULL) rx_buffer = malloc(max_buffer_size);

	ssize_t rx_size = 0;

	while ((rx_size = recv(consumers[consumer_index], rx_buffer, max_buffer_size, MSG_DONTWAIT)) > 0) {
		printf("Consumer[%zd] Got '%.*s'\n", consumer_index, (int)rx_size, (char *)rx_buffer);
	}

	if (errno == EWOULDBLOCK) return;

	if (rx_size < 0) {
		printf("Consumer[%zd] Error waiting for response: %s\n", consumer_index, strerror(errno));
	}

	return;
}

static int create_socket(size_t consumer_index)
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(get_consumer_port(consumer_index));
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
		err(1, "Bind on consumer %zd", consumer_index);
	}

	return sock;
}

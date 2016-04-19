/*
 * common.h
 *
 *  Created on: 15 Apr 2016
 *      Author: itay
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stdbool.h>

#include <errno.h>
#include <err.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#define MULTICAST_PORT 9910

#define CONSUMER_SRC_PORT 9920
#define CONSUMER_DST_PORT 10000

#endif /* COMMON_H_ */

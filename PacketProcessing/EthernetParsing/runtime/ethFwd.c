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

#include <MaxSLiCInterface.h>
#include <MaxSLiCNetInterface.h>

extern max_file_t *EthFwd_init();

int main(int argc, char *argv[]) {
	if (argc < 4) {
		printf("Syntax: %s <TOP local IP> <BOT local IP> <forward IP>\n", argv[0]);
		return 1;
	}

	struct in_addr top_ip;
	struct in_addr bot_ip;
	struct in_addr fwd_ip;
	struct in_addr netmask;

	inet_aton(argv[1], &top_ip);
	inet_aton(argv[2], &bot_ip);
	inet_aton(argv[3], &fwd_ip);
	inet_aton("255.255.255.0", &netmask);

	uint16_t port = 7653;

	printf("EthFwd: TOP IP '%s', BOT IP '%s', Forward IP '%s', port %u\n", argv[1], argv[2], argv[3], port);

	max_file_t *maxfile = EthFwd_init();
	max_engine_t * engine = max_load(maxfile, "*");

	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &top_ip, &netmask);
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &bot_ip, &netmask);

	struct ether_addr local_mac2, remote_mac2;
	max_arp_lookup_entry(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &fwd_ip, &remote_mac2);
	max_eth_get_default_mac_address(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &local_mac2);

	uint64_t localMac = 0, forwardMac = 0;
	memcpy(&localMac, &local_mac2, 6);
	memcpy(&forwardMac, &remote_mac2, 6);

	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_set_uint64t(action, "fwdKernel", "localIp", bot_ip.s_addr);
	max_set_uint64t(action, "fwdKernel", "forwardIp", fwd_ip.s_addr);
	max_set_uint64t(action, "fwdKernel", "localMac", localMac);
	max_set_uint64t(action, "fwdKernel", "forwardMac", forwardMac);
	max_set_uint64t(action, "fwdKernel", "port", port);
	max_run(engine, action);

	printf("JDFE Running.\n");
	getchar();

	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n");
	return 0;
}

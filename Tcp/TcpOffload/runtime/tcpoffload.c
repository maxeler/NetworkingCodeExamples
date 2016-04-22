#define _GNU_SOURCE

#include <errno.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "MaxSLiCInterface.h"

extern max_file_t *TcpOffload_init();

static struct in_addr netmask;
static struct in_addr dfe_top_ip;

int main(int argc, char *argv[]) {

	inet_aton("255.255.255.0", &netmask);
	printf("Local DFE Top @ %s\n", argv[1]);
	inet_aton(argv[1], &dfe_top_ip);

	max_file_t *maxfile = TcpOffload_init();
	max_engine_t * engine = max_load(maxfile, "*");


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_run(engine, action);




	



	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

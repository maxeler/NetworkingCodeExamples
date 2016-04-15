#define _GNU_SOURCE

#include <errno.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "MaxSLiCInterface.h"

extern max_file_t *UdpForwarding_init();

int main(int argc, char *argv[]) {

	max_file_t *maxfile = UdpForwarding_init();
	max_engine_t * engine = max_load(maxfile, "*");


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_run(engine, action);


	/*
	 * More code here
	 */

	



	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

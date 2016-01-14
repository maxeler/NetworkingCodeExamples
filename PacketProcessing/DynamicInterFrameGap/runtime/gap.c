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
#include <err.h>


#include "MaxSLiCInterface.h"

static void dump(void *v, size_t size) {
	uint64_t *b = v;
	for (size_t i=0; i < (size / sizeof(uint64_t)); i++) {
		printf("[%zd] 0x%lx\n", i, b[i]);
	}
}


int main(int argc, char *argv[]) {

	max_file_t *maxfile = Gap_init();
	max_engine_t * engine = max_load(maxfile, "*");


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_run(engine, action);


	size_t bufferSize = 4096 * 4096;
	void *inBuffer = NULL;
	void *outBuffer = NULL;
	if (posix_memalign(&inBuffer, 4096, bufferSize)) {
		err(1, "Couldn't allocation input buffer");
	}
	if (posix_memalign(&outBuffer, 4096, bufferSize)) {
		err(1, "Couldn't allocation output buffer");
	}
	max_framed_stream_t *inFrame = max_framed_stream_setup(engine, "src", inBuffer, bufferSize, 2048-16);
	max_framed_stream_t *outFrame = max_framed_stream_setup(engine, "dst", outBuffer, bufferSize, -1);

	// Now, stream in some frames and see what happens.

	for (size_t i=0 ; i < 8; i++) {
		void *f;
		while (max_framed_stream_write_acquire(inFrame, 1, &f) != 1) usleep(10);

		uint8_t *inputData = f;

		/*
		 * Request a gap every other packet
		 */
		inputData[20] = i % 2 == 1 ? 'G' : 'N';

		size_t frameSize = 60;
		printf("Sending frame %zd\n", i);
		max_framed_stream_write(inFrame, 1, &frameSize);


		void *oFrame;
		size_t oFrameSize;
		while (max_framed_stream_read(outFrame, 1, &oFrame, &oFrameSize) != 1) usleep(10);

		printf("Got frame %zd - %zd bytes (Expecting %zd)\n", i, oFrameSize, frameSize);

		dump(oFrame, oFrameSize);


		max_framed_stream_discard(outFrame, 1);
	}



	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n");
	return 0;
}

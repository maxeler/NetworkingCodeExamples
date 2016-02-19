#define _DEFAULT_SOURCE
#define __USE_XOPEN2K

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <features.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MaxSLiCInterface.h"
#include "MaxSLiCNetInterface.h"


typedef struct __attribute__((packed)) {
	uint32_t  cpuData1;
	uint8_t   cpuData2;
	uint32_t : 24; // padding
} CpuInfoType_t;

typedef struct __attribute__((packed)) {
	uint8_t   hfa;
	uint64_t  hfb;
	uint64_t  hfc : 48;
	uint32_t  hfd;
} HeaderType_t;


typedef struct __attribute__((packed)) {
	uint16_t index;
	uint8_t data[32];
} FrameFormat_t;


void dump(void *v, size_t size) {
	uint64_t *b = v;
	for (size_t i=0; i < (size / sizeof(uint64_t)); i++) {
		printf("[%zd] 0x%lx\n", i, b[i]);
	}
}

int main(int argc, char *argv[]) {

	max_file_t *maxfile = HeaderInserter_init();
	max_engine_t * engine = max_load(maxfile, "*");

	printf("FrameFormat_t size = %zd, HeaderType_t size = %zd\n", sizeof(FrameFormat_t), sizeof(HeaderType_t));


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	/*
	 * Fill up Rom for 16 indices
	 */

	max_actions_t *action = max_actions_init(maxfile, NULL);
	for (size_t i=0; i < 16; i++) {
		CpuInfoType_t info;
		info.cpuData1 = 0x1000 + i;
		info.cpuData2 = 0x22;
		uint64_t d = *(uint64_t *)&info;
		max_set_mem_uint64t(action, "HeaderKernel", "info", i, d);
	}
	max_run(engine, action);


	size_t bufferSize = 4096 * 4096;
	void *inBuffer = malloc(bufferSize);
	void *outBuffer = malloc(bufferSize);
//	posix_memalign(&inBuffer, 4096, bufferSize);
//	posix_memalign(&outBuffer, 4096, bufferSize);
	max_framed_stream_t *inFrame = max_framed_stream_setup(engine, "inFrame", inBuffer, bufferSize, 2048-16);
	max_framed_stream_t *outFrame = max_framed_stream_setup(engine, "outFrame", outBuffer, bufferSize, -1);

	// Now, stream in some frames and see what happens.
	size_t frameSize = sizeof(FrameFormat_t);

	for (size_t i=0 ; i < 20; i++) {
		void *f;

		while (max_framed_stream_write_acquire(inFrame, 1, &f) != 1) usleep(10);

		FrameFormat_t *ff = f;
		ff->index = 3; //i % 16;

		size_t dataSize = 10 + i;
		for (size_t s=0; s < dataSize; s++) {
			ff->data[s] = s & 0xFF;
		}

		printf("Sending: index %d, data: ", ff->index);
		for (size_t s=0; s < dataSize; s++) {
			printf("%d ",ff->data[s]);
		}
		printf("\n");

		max_framed_stream_write(inFrame, 1, &frameSize);
	}

	for (size_t i=0; i < 20; i++) {
		size_t dataSize = 10 + i;
		void *oFrame;
		size_t oFrameSize;
		while (max_framed_stream_read(outFrame, 1, &oFrame, &oFrameSize) != 1) usleep(100);

		printf("Got frame - %zd bytes (Expecting %zd)\n", oFrameSize, frameSize + sizeof(HeaderType_t));

		dump(oFrame, oFrameSize);

		HeaderType_t *header = oFrame;
		printf("Header is: a=0x%x, b=0x%lx, c=0x%lx, d=0x%x\n", header->hfa, header->hfb, (uint64_t)header->hfc, header->hfd);
		FrameFormat_t *off = (FrameFormat_t *)(header + 1);
		printf("Index: %d, data: ", off->index);
		for (size_t s=0; s < dataSize; s++) {
			printf("%d ", off->data[s]);
		}
		printf("\n");

		max_framed_stream_discard(outFrame, 1);
	}



	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n");
	return 0;
}

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
#include <stdbool.h>

#include "MaxSLiCInterface.h"
#include "MaxSLiCNetInterface.h"


typedef struct __attribute__((packed)) {
	uint32_t  cpuData1;
	uint32_t  cpuData2 : 8;
	uint32_t : 24;
} CpuInfoType_t;

typedef struct __attribute__((packed)) {
	uint8_t   hfa;
	uint64_t  hfb;
	uint64_t  hfc : 48;
	uint32_t  hfd;
} HeaderType_t;

#define ARRAY_SIZE 32

typedef struct __attribute__((packed)) {
	uint16_t index;
} FrameHeader_t;

typedef struct __attribute__((packed)) {
	FrameHeader_t header;
	uint8_t data[ARRAY_SIZE];
} FrameFormat_t;

static size_t div_by_8_round_up(size_t size) {
	return ((size + 7) & ~7) >> 3;
}

void dump(void *v, size_t size) {
	uint64_t *b = v;
	size_t sizeWords = div_by_8_round_up(size);
	for (size_t i=0; i < sizeWords; i++) {
		printf("[%zd] 0x%lx\n", i, b[i]);
	}
}

int main(int argc, char *argv[]) {

	max_file_t *maxfile = HeaderInserter_init();
	max_engine_t * engine = max_load(maxfile, "*");
	max_reset_engine(engine);

	printf("FrameFormat_t size = %zd, HeaderType_t size = %zd\n", sizeof(FrameFormat_t), sizeof(HeaderType_t));


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	/*
	 * Fill up Rom for 16 indices
	 */

	CpuInfoType_t *cpuRom = malloc(sizeof(CpuInfoType_t) * 16);
	max_actions_t *action = max_actions_init(maxfile, NULL);
	uint64_t d;
	for (uint32_t i=0; i < 16; i++) {
		CpuInfoType_t *info = &cpuRom[i];
		memset(info, 0, sizeof(CpuInfoType_t));
		info->cpuData1 = 0x1000 + (uint32_t)i;
		info->cpuData2 = 0x22;
		memcpy(&d, info, sizeof(CpuInfoType_t));
		printf("Setting ROM index[%d] -> d1 = 0x%x d2 = 0x%x --> 0x%lx\n", i, info->cpuData1, info->cpuData2, d);
		max_set_mem_uint64t(action, "HeaderKernel", "info", i, d);
	}
	max_run(engine, action);

	size_t bufferSize = 4096 * 4096;
	void *inBuffer;
	void *outBuffer;
	posix_memalign(&inBuffer, 4096, bufferSize);
	posix_memalign(&outBuffer, 4096, bufferSize);
	max_framed_stream_t *inFrame = max_framed_stream_setup(engine, "inFrame", inBuffer, bufferSize, 2048-16);
	max_framed_stream_t *outFrame = max_framed_stream_setup(engine, "outFrame", outBuffer, bufferSize, -1);

	size_t numFrames = ARRAY_SIZE;

	for (size_t i=0 ; i <= numFrames; i++) {
		void *f;
		FrameFormat_t *ff;

		uint16_t index = 3;

		{

			while (max_framed_stream_write_acquire(inFrame, 1, &f) != 1) usleep(10);

			ff = f;
			ff->header.index = index;

			size_t dataSize = i;
			for (size_t s=0; s < dataSize; s++) {
				ff->data[s] = s & 0xFF;
			}

			printf("Index %d, data: ", ff->header.index);
			for (size_t s=0; s < dataSize; s++) {
				printf("%d ",ff->data[s]);
			}
			printf("\n");

			size_t inFrameSize = sizeof(FrameHeader_t) + dataSize;
			printf("----------------\nSending frame - %zd bytes\n", inFrameSize);
			dump(f, inFrameSize);
			printf("----------------\n");
			max_framed_stream_write(inFrame, 1, &inFrameSize);
		}

		{
			size_t dataSize = i;
			void *oFrame;
			size_t frameSize;
			bool fail = false;
			while (max_framed_stream_read(outFrame, 1, &oFrame, &frameSize) != 1) usleep(100);

			size_t expectedSize = sizeof(HeaderType_t) + sizeof(FrameHeader_t) + dataSize;
			printf("Got frame %zd...\n", i);
			if (frameSize != expectedSize) {
				printf("Size mismatch. Expected %zd bytes, got %zd bytes\n", expectedSize, frameSize);
				fail = true;
			} else {
				printf("Size = %zd bytes\n", frameSize);
			}


			dump(oFrame, frameSize);

			HeaderType_t *header = oFrame;
			CpuInfoType_t *cpuInfo = &cpuRom[index];
			HeaderType_t expectedHeader = {
					.hfa = cpuInfo->cpuData2,
					.hfb = 0xAABBCCDDEEFFUL,
					.hfc = 0xCCCCCCCCCCCCUL,
					.hfd = cpuInfo->cpuData1
			};

			if (memcmp(&expectedHeader, header, sizeof(HeaderType_t))) {
				printf("Header mismatch!\n");
				printf("Got     : a=0x%x, b=0x%lx, c=0x%lx, d=0x%x\n", header->hfa, header->hfb, (uint64_t)header->hfc, header->hfd);
				printf("Expected: a=0x%x, b=0x%lx, c=0x%lx, d=0x%x\n", expectedHeader.hfa, expectedHeader.hfb, (uint64_t)expectedHeader.hfc, expectedHeader.hfd);
				fail = true;
			} else if (fail) printf("Header data is good.\n");


			FrameFormat_t *off = (FrameFormat_t *)(header + 1);
			if (memcmp(ff, off, sizeof(FrameHeader_t) + dataSize) != 0) {
				printf("Frame data is different:\n");
				if(ff->header.index != off->header.index) {
					printf("Rom Index: expected %d, got %d\n", ff->header.index, off->header.index);
				}
				for (size_t s=0; s < dataSize; s++) {
					if(ff->data[s] != off->data[s]) {
						printf("Data[%zd]: expected 0x%x, got 0x%x\n", s, ff->data[s], off->data[s]);
					}
				}
				printf("\n");
				fail = true;
			} else if (fail) printf("However, frame data is good.\n");


			max_framed_stream_discard(outFrame, 1);

			if (fail) {
				printf("Failed!\n");
				exit(1);
			} else printf("Good.\n");
		}
	}



	max_unload(engine);
	max_file_free(maxfile);

	printf("Done.\n");
	return 0;
}

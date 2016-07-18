#define _GNU_SOURCE

#include <errno.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "MaxSLiCInterface.h"
#include "MaxSLiCNetInterface.h"

extern max_file_t *RomFrameGen_init();

#define MAC_WIDTH_BYTES sizeof(uint32_t)
#define WORD_SIZE_BYTES MAC_WIDTH_BYTES
#define NUM_SUPPORTED_MESSAGE_TYPES 20
#define MAX_MESSAGE_SIZE_WORDS 16
#define MAX_MESSAGE_SIZE_BYTES (MAX_MESSAGE_SIZE_WORDS * WORD_SIZE_BYTES)

static size_t *message_size_per_type;
static uint32_t *frameRom;

union meta_union {
	struct {
		uint16_t start_index;
		uint16_t end_index;
		uint8_t frame_mod;
	} metadata;
	uint64_t raw;
};

static size_t bytes_to_words(size_t bytes) {
	size_t words = bytes / WORD_SIZE_BYTES;
	words += (bytes % WORD_SIZE_BYTES == 0 ? 0 : 1);
	return words;
}

static uint8_t frame_mod(size_t size) {
	return (uint8_t)(size % WORD_SIZE_BYTES);
}

static uint32_t apply_mod(uint32_t v, uint8_t mod) {
	uint32_t n = 0;
	memcpy(&n, &v, mod == 0 ? sizeof(v) : mod);
	return n;
}

static void configure_roms(max_actions_t *action) {
	message_size_per_type = malloc(sizeof(size_t) * NUM_SUPPORTED_MESSAGE_TYPES);
	frameRom = malloc(MAX_MESSAGE_SIZE_BYTES * NUM_SUPPORTED_MESSAGE_TYPES);

	for (size_t t=0; t < NUM_SUPPORTED_MESSAGE_TYPES; t++) {
		message_size_per_type[t] = rand() % MAX_MESSAGE_SIZE_BYTES;

		/*
		 * The metadata
		 */
		size_t message_size_words = bytes_to_words(message_size_per_type[t]);
		union meta_union u;
		u.metadata.start_index = MAX_MESSAGE_SIZE_WORDS * t;
		u.metadata.end_index = u.metadata.start_index + message_size_words - 1;
		u.metadata.frame_mod = frame_mod(message_size_per_type[t]);

		max_set_mem_uint64t(action, "RomFrameGenKernel", "metadataRom", t, u.raw);

		/*
		 * The actual frame data
		 */
		for (uint16_t index = u.metadata.start_index; index <= u.metadata.end_index; index++) {
			frameRom[index] = rand();
			uint32_t theData = frameRom[index];
			if (index == u.metadata.end_index) theData = apply_mod(theData, u.metadata.frame_mod);
			max_set_mem_uint64t(action, "RomFrameGenKernel", "frameRom", index, (uint64_t)theData);
		}
	}
}


void dump(void *v, size_t size) {
	uint64_t *b = v;
	size_t sizeWords = bytes_to_words(size);
	for (size_t i=0; i < sizeWords; i++) {
		printf("[%zd] 0x%lx\n", i, b[i]);
	}
}

uint32_t *get_frame_buffer(uint8_t message_type) {
	if (message_type > NUM_SUPPORTED_MESSAGE_TYPES) return NULL;
	return &frameRom[MAX_MESSAGE_SIZE_WORDS * message_type];
}

int main(int argc, char *argv[]) {

	max_file_t *maxfile = RomFrameGen_init();
	max_engine_t * engine = max_load(maxfile, "*");

	struct timeval t;
	gettimeofday(&t, NULL);

	int rand_seed = (int)t.tv_sec;
	printf("Using random seed: %d\n", rand_seed);
	srand(rand_seed);



	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);


	max_actions_t *action = max_actions_init(maxfile, NULL);
	configure_roms(action);
	max_run(engine, action);


	void *src_buffer, *dst_buffer;
	size_t buffer_size = 4096 * 512;
	posix_memalign(&src_buffer, 4096, buffer_size);
	posix_memalign(&dst_buffer, 4096, buffer_size);

	size_t max_frame_size = 2000;
	max_framed_stream_t* src = max_framed_stream_setup(engine, "src", src_buffer, buffer_size, max_frame_size);
	max_framed_stream_t* dst = max_framed_stream_setup(engine, "dst", dst_buffer, buffer_size, -1);


	size_t num_frames = 10;
	for (size_t i=0; i < num_frames; i++) {

		void *src_frame = NULL;
		while (max_framed_stream_write_acquire(src, 1, &src_frame) == 0) usleep(10);


		uint8_t *src_frame_bytes = src_frame;

		/*
		 * First byte of the data is the message type
		 */
		uint8_t message_type = i % NUM_SUPPORTED_MESSAGE_TYPES;
		src_frame_bytes[0] = message_type;
		size_t write_frame_size = 1 + (i % max_frame_size);

		uint16_t start = MAX_MESSAGE_SIZE_WORDS * (uint16_t)message_type;
		uint16_t end = start + bytes_to_words(message_size_per_type[message_type]) - 1;
		uint8_t mod = frame_mod((size_t)message_size_per_type[message_type]);
		printf("----------------\nSending message type %d (start %d, end %d, mod %d)\n", message_type, start, end, mod);


		max_framed_stream_write(src, 1, &write_frame_size);

		/*
		 * Read back the response
		 */

		void *dst_frame = NULL;
		size_t dst_frame_size = 0;
		while (max_framed_stream_read(dst, 1, &dst_frame, &dst_frame_size) == 0) usleep(10);

		if (dst_frame_size != message_size_per_type[message_type]) {
			printf("Frame size mismatch: Expected %zd, got %zd\n", message_size_per_type[message_type], dst_frame_size);
			exit(1);
		}

		if (memcmp(dst_frame, get_frame_buffer(message_type), dst_frame_size)) {
			printf("Frames mismatch!\n");
			printf("Expected: \n");
			dump(get_frame_buffer(message_type), message_size_per_type[message_type]);
			printf("Got:\n");
			dump(dst_frame, dst_frame_size);
			exit(1);
		}

		printf("Good\n");

		max_framed_stream_discard(dst, 1);
	}




	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

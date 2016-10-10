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

extern max_file_t *Fragmenter_init();

#define LINK_WIDTH_BYTES 8
#define WORD_SIZE LINK_WIDTH_BYTES
#define MAX_FRAME_SIZE_BYTES 1400
#define MAX_FRAGMENT_SIZE_BYTES 150

static size_t genFrameData(uint8_t *buffer, size_t max_size) {
	size_t frame_size_bytes = 1 + (rand() % (max_size - 1));
	for (size_t i=0; i < frame_size_bytes; i++) {
		buffer[i] = rand() & 0xFF;
	}
	return frame_size_bytes;
}

#define MAX_ACC_SIZE 4096
static size_t pos;
static uint8_t *accbuffer;

static void reset_buffer() {
	pos = 0;
}

static size_t get_acc_buffer_size() {
	return pos;
}

static void *get_acc_buffer() {
	return accbuffer;
}

static void accumulate(void *part, size_t part_size) {
	if (pos > MAX_ACC_SIZE) {
		printf("Accumulation buffer size exceeded!\n");
		exit(-1);
	}

	memcpy(accbuffer + pos, part, part_size);
	pos += part_size;
}

static void dump_buffer(void *buffer, size_t buffer_size) {
	printf("%p (%zd bytes): ", buffer, buffer_size);

	uint8_t *bytes = buffer;
	for (size_t i=0; i < buffer_size; i++) {
		printf("%02X ", bytes[i]);
		if (i % 8 == 7) printf("\n");
	}

	printf("\n");

}

static size_t expected_fragments(size_t src_frame_size_bytes, size_t max_fragment_size_words) {
	if (max_fragment_size_words == 0) {
		printf("Max fragment size has to be > 0\n");
		exit(1);
	}
	size_t actual_max_frag_size = max_fragment_size_words * WORD_SIZE;
	size_t num_fragments = (src_frame_size_bytes / actual_max_frag_size) + (src_frame_size_bytes % actual_max_frag_size != 0 ? 1 : 0);
	return num_fragments;
}

int main(int argc, char *argv[]) {

	max_file_t *maxfile = Fragmenter_init();
	max_engine_t * engine = max_load(maxfile, "*");

	uint32_t seed = time(NULL);
	printf("Using random seed: 0x%08X\n", seed);
	srand(seed);


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	size_t max_frame_fragment_size_words = MAX_FRAGMENT_SIZE_BYTES / WORD_SIZE;


	/*
	 * Set maximum frame size in words
	 * This can also be hard-coded in the maxfile
	 */
	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_set_uint64t(action, "Fragmenter_i0_sCustom", "maxFrameSizeWords", max_frame_fragment_size_words);
	max_run(engine, action);
	printf("Max Fragment size %zd bytes (%zd words)\n", MAX_FRAGMENT_SIZE_BYTES, max_frame_fragment_size_words);


	accbuffer = malloc(MAX_ACC_SIZE);
	size_t buffer_size = 512 * 4096;
	void *src_buffer = malloc(buffer_size);
	void *dst_buffer = malloc(buffer_size);
	max_framed_stream_t* src_stream = max_framed_stream_setup(engine, "src", src_buffer, buffer_size, MAX_FRAME_SIZE_BYTES);
	max_framed_stream_t* dst_stream = max_framed_stream_setup(engine, "dst", dst_buffer, buffer_size, -1);
	

	size_t num_frames = 10000;

	for (size_t f = 0; f < num_frames; f++) {
		printf("Getting buffer...");
		// Acquire source frame buffer
		void *src = NULL;
		while (max_framed_stream_write_acquire(src_stream, 1, &src) != 1) usleep(10);

		// Generate a new frame data
		size_t src_frame_size = genFrameData(src, MAX_FRAME_SIZE_BYTES);

		printf("Sending frame %zd (%zd bytes)... ", f, src_frame_size);
		// Send it in
		max_framed_stream_write(src_stream, 1, &src_frame_size);


		size_t expected_frags = expected_fragments(src_frame_size, max_frame_fragment_size_words);
		printf("Expecting %zd fragments... ", expected_frags);
		// Accumulate fragments
		reset_buffer();
		size_t frag_count = 0;

		while (get_acc_buffer_size() < src_frame_size) {
			void *fragment;
			size_t fragment_size;
			if (max_framed_stream_read(dst_stream, 1, &fragment, &fragment_size) == 1) {
				accumulate(fragment, fragment_size);
				frag_count++;
				printf(" %zd", frag_count);
				max_framed_stream_discard(dst_stream, 1);
			}
		}

		if (frag_count != expected_frags) {
			printf("Got too many fragments!\n");
			exit(1);
		}
		printf("... ");

		// Compare accumulated fragments to original
		if (memcmp(get_acc_buffer(), src, src_frame_size) != 0) {
			printf("Buffer do not match\n");
			printf("Source buffer ");
			dump_buffer(src, src_frame_size);

			printf("Accumulated buffer ");
			dump_buffer(get_acc_buffer(), src_frame_size);
			exit(1);
		}
		printf("all good.\n");
	}

	printf("Test passed!\n");

	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

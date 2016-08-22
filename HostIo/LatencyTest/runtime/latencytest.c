#include <errno.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "MaxSLiCInterface.h"
#define PAGE_SIZE 4096

extern max_file_t *LatencyTest_init();

#define MS_PER_SEC 1000.0
#define US_PER_SEC 1000000.0
#define NS_PER_SEC 1000000000.0
#define PS_PER_SEC 1000000000000.0

static double ps_per_cycle;
static double ns_per_cycle;
static double us_per_cycle;
static double ms_per_cycle;

/*
 * Read Time-Stamp Counter
 * Based on: http://www.mcs.anl.gov/~kazutomo/rdtsc.html
 */
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static void calc_cpufreq() {
	uint64_t start, finish;
	printf("Measuring CPU clock frequency..."); fflush(stdout);

	start = rdtsc();
	sleep(10);
	finish = rdtsc();

    ps_per_cycle = (PS_PER_SEC * 10.0) / ((double)finish-start);
    ns_per_cycle = (NS_PER_SEC * 10.0) / ((double)finish-start);
    us_per_cycle = (US_PER_SEC * 10.0) / ((double)finish-start);
    ms_per_cycle = (MS_PER_SEC * 10.0) / ((double)finish-start);
    uint64_t freq_hz = (uint64_t)(((double)finish-start) / 10.0);

    printf(" %.3f [MHz] (%.2f ps per cycle)\n", freq_hz / 1000000.0, ps_per_cycle);
}

int main(int argc, char *argv[]) {

	max_file_t *maxfile = LatencyTest_init();
	max_engine_t * engine = max_load(maxfile, "*");

	calc_cpufreq();

	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	max_actions_t *action = max_actions_init(maxfile, NULL);
	max_run(engine, action);


	/*
	 * Setup up low latency streams
	 */

	const size_t slot_size_bytes = 512;
	const size_t num_slots = 512;
	void *src_buffer, *dst_buffer;
	posix_memalign(&src_buffer, PAGE_SIZE, slot_size_bytes * num_slots);
	posix_memalign(&dst_buffer, PAGE_SIZE, slot_size_bytes * num_slots);

	max_llstream_t *src_stream = max_llstream_setup(engine, "src", num_slots, slot_size_bytes, src_buffer);
	max_llstream_t *dst_stream = max_llstream_setup(engine, "dst", num_slots, slot_size_bytes, dst_buffer);
	

	uint64_t min_time = UINT64_MAX;

	size_t iterations = 1000000;
	for (size_t i=0; i < iterations; i++) {
		void *dst_slot;
		void *src_slot;

		// Acquire a write slot
		while (max_llstream_write_acquire(src_stream, 1, &src_slot) != 1) usleep(1);

		// fill it up with some data
		uint8_t *src_slot_data = src_slot;
		for (size_t d = 0; d < slot_size_bytes; d++) {
			src_slot_data[d] = rand();
		}

		// Start the timer then send the slot to the DFE
		uint64_t start = rdtsc();
		max_llstream_write(src_stream, 1);
		while (max_llstream_read(dst_stream, 1, &dst_slot) != 1)
			/* No sleep, as we are measuring latency */;
		// Got data, so stop the timer
		uint64_t finish = rdtsc();

		// Compare the data
		if (memcmp(src_slot, dst_slot, slot_size_bytes)) {
			printf("Data mismatch!\n");
			exit(1);
		}

		uint64_t time_now = finish - start;
		if (time_now < min_time) {
			min_time = time_now;
			printf("Best round-trip time so far %.2f nanoseconds\n", time_now * ns_per_cycle);
		}

		// Discard the slot, it's free now.
		max_llstream_read_discard(dst_stream, 1);
	}


	printf("CPU <-> DFE Round-Trip latency is %.2f\n", min_time * ns_per_cycle);


	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

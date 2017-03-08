#define _GNU_SOURCE

#include <errno.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


#include "MaxSLiCInterface.h"
#include "MaxSLiCNetInterface.h"

struct __attribute__((packed)) mdupdate {
	uint64_t  timestamp;
	uint32_t  securityId;
	struct {
		uint32_t  price;
		uint32_t  qty;
	} trade;
	struct {
		struct {
			uint32_t  buyPrice;
			uint32_t  buyQty;
			uint32_t  buyNumberOfOrders;
			uint32_t  sellPrice;
			uint32_t  sellQty;
			uint32_t  sellNumberOfOrders;
		} level[5];
	} orderBook;
	uint32_t   tradeValid : 1;
	uint32_t   bookValid : 1;
	uint32_t  padding : 30;
};

static size_t md_padding_size = 112;


struct __attribute__((packed)) order_entry {
	uint32_t  securityId;
	uint8_t   side;
	uint32_t  qty;
	uint32_t  price;
	uint8_t padding[3];
};

struct __attribute__((packed)) status {
	double  ema;
	double  vwap;
	double  liveness;
	double  bestPrice;
	double  emaFactor;
	double  vwapFactor;
	double  execute;
	uint32_t securityId;
	uint32_t side : 8;
	uint32_t updateNum : 24;
};

static bool running = true;
extern max_file_t *TradingStartegyExample_init();

static max_llstream_t *setupStream(max_engine_t *engine, const char *name, size_t slot_size) {
	size_t buffer_size = slot_size * 512;
	void *buffer;
	if (posix_memalign(&buffer, 4096, buffer_size)) {
		perror("Couldn't allocate memory for framed streams.");
	}

	printf("Setting up '%s' stream - slot size %zd\n", name, slot_size);
	return max_llstream_setup(engine, name, 512, slot_size, buffer);
}


static max_llstream_t *setupMarketStream(max_engine_t *engine) {
	return setupStream(engine, "marketUpdates", sizeof(struct mdupdate) + md_padding_size);
}


static max_llstream_t *setupOrderEntryStream(max_engine_t *engine) {
	return setupStream(engine, "orderEntry", sizeof(struct order_entry));
}

static max_llstream_t *setupStatusStream(max_engine_t *engine) {
	return setupStream(engine, "status", sizeof(struct status));
}

static size_t getNextMarketUpdate(char *filename, struct mdupdate *update) {
	static FILE *f = NULL;

	if (f == NULL) {
		f = fopen(filename, "r");
		if (f == NULL) {
			perror("Failed to open md file");
			exit(1);
		}
	}

	return fread(update, sizeof(struct mdupdate), 1, f);
}

static void pushUpdate(max_llstream_t *stream, struct mdupdate *update) {
	void *slot = NULL;
	while (max_llstream_write_acquire(stream, 1, &slot) != 1) {
		usleep(10);
	}

	memcpy(slot, update, sizeof(struct mdupdate));

	max_llstream_write(stream, 1);
}

static bool pollOrderEntry(max_llstream_t *stream, struct order_entry *entry) {
	void *slot;
	if (max_llstream_read(stream, 1, &slot) == 1) {
		memcpy(entry, slot, sizeof(struct order_entry));
		max_llstream_read_discard(stream, 1);
		return true;
	}
	return false;
}

static void *orderEntryThread(void *arg) {
	max_llstream_t *order_stream = arg;
	struct order_entry order_entry;

	while (running) {
		if (pollOrderEntry(order_stream, &order_entry)) {
//			printf("Order [%u] %u @ %u\n", order_entry.securityId, order_entry.qty, order_entry.price);
		} else {
			usleep(10);
		}
	}
}

static void *statusThread(void *arg) {
	void *slot;
	max_llstream_t *status_stream = arg;
	struct status *status;
	FILE *stats = fopen("stats.csv", "w");

	fprintf(stats, "bestPrice, ema, vwap, liveness, execute\n");

	while (running) {
		if (max_llstream_read(status_stream, 1, &slot) == 1) {
			status = slot;

			printf("[%u] Status for sec[%u]: ema=%.3f, vwap=%.3f, liveness=%.3f, bestPrice=%.3f --> execute=%.3f\n",
					status->updateNum,
					status->securityId,
					status->ema,
					status->vwap,
					status->liveness,
					status->bestPrice,
					status->execute);

			fprintf(stats, "%.3f, %.3f, %.3f, %.3f, %.3f\n",
					 status->bestPrice, status->ema, status->vwap, status->liveness, status->execute);

			max_llstream_read_discard(status_stream, 1);
		} else {
			usleep(10);
		}
	}

	fclose(stats);
}

static void configure_params(max_file_t *maxfile, max_engine_t *engine)
{
	max_actions_t *action = max_actions_init_explicit(maxfile);
	max_set_uint64t(action, "StrategyKernel", "secId0", 123456);
	max_set_uint64t(action, "StrategyKernel", "secId1", 999999);
	max_set_uint64t(action, "StrategyKernel", "secId2", 888888);
	max_set_uint64t(action, "StrategyKernel", "secId3", 777777);

	max_set_double(action, "StrategyKernel", "emaAlphaBUY", 1 / 100.0);
	max_set_double(action, "StrategyKernel", "livenessCBUY", 1 / 10.0);
	max_set_double(action, "StrategyKernel", "cEmaBUY", 1.0);
	max_set_double(action, "StrategyKernel", "cVwapBUY", 1.0);
	max_set_double(action, "StrategyKernel", "cLivenessBUY", 1.0);
	max_set_double(action, "StrategyKernel", "executeThresholdBUY", 2.5);


	max_set_double(action, "StrategyKernel", "emaAlphaSELL", 1 / 100.0);
	max_set_double(action, "StrategyKernel", "livenessCSELL", 1 / 10.0);
	max_set_double(action, "StrategyKernel", "cEmaSELL", 1.0);
	max_set_double(action, "StrategyKernel", "cVwapSELL", 1.0);
	max_set_double(action, "StrategyKernel", "cLivenessSELL", 1.0);
	max_set_double(action, "StrategyKernel", "executeThresholdSELL", 2.5);

	max_run(engine, action);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Syntax: %s <market data file path>\n", argv[0]);
		return 1;
	}

	char *filename = argv[1];
	printf("Using market data from '%s'\n", filename);
	max_file_t *maxfile = TradingStartegyExample_init();
	max_engine_t *engine = max_load(maxfile, "*");


	max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

	configure_params(maxfile, engine);


	max_llstream_t *order_stream = setupOrderEntryStream(engine);
	max_llstream_t *md_stream = setupMarketStream(engine);
	max_llstream_t *status_stream = setupStatusStream(engine);


	struct mdupdate update;

	pthread_t order_thread, status_thread;
	pthread_create(&order_thread, NULL, orderEntryThread, order_stream);
	pthread_create(&status_thread, NULL, statusThread, status_stream);


	for (size_t i=0; i < 2000 && getNextMarketUpdate(filename, &update) == 1; i++) {
		pushUpdate(md_stream, &update);
	}

	sleep(1);
	running = false;

	pthread_join(order_thread, NULL);
	pthread_join(status_thread, NULL);


	max_unload(engine);
	max_file_free(maxfile);

	return 0;
}

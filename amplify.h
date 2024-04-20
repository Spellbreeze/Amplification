#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <err.h>

#include "configurations.h"
#include "fr_util.h"

#ifndef AMPLIFY_H_
#define AMPLIFY_H_

void map_or_die(ADDR_PTR addr, size_t sz); // pulled from leaky.page
uint64_t leaky_run(ADDR_PTR *cacheSet, ADDR_PTR evict, ADDR_PTR *clearSet, int repetitions, int dist, int refresh); // pulled from leaky.page

uint64_t leaky_run_b4(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh);
uint64_t leaky_run_b3(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh);
uint64_t leaky_run_b2(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh);

uint64_t simple_run(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh);

uint64_t leaky_refresh_b4(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash); // Refreshing Appears to be largely ineffective
uint64_t leaky_refresh_b3(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash); // Refreshing Appears to be largely ineffective
uint64_t leaky_refresh_b2(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash); // Refreshing Appears to be largely ineffective

uint64_t showtime_reorder(ADDR_PTR *cacheSet, int reorder, ADDR_PTR *clearSet, int repetitions, int dist, int refresh);
uint64_t showtime_llc_eviction(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR llc_candidate, ADDR_PTR l1b_element, int flushed, int repetitions, int dist, int refresh);

#endif
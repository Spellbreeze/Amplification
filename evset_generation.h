#include <stdbool.h>
#include <stdint.h>

#include "list_utils.h"

#ifndef EVSET_GENERATION_H_
#define EVSET_GENERATION_H_

void        construct_L1_eviction_set(uint64_t* evset, uint64_t* page, uint64_t target, uint8_t SIZE, uint64_t STRIDE, uint64_t MAX_RAND);
uint64_t    get_new_random_index(uint64_t* rands, uint64_t MAX_RAND, int nb_so_far);

void generate_llc_eviction_set(uint64_t target_addr, int ways, uint64_t *evict_mem, int thrDET, uint64_t *evsetLLC);
int ps_evset(Elem **evset, char *victim, int len, uint64_t* page, int threshold);
int ps_evset_reduce(Elem **evset, char *victim, int len, int threshold);
int ps_evset_test(Elem **evset, char *victim, int threshold, int test_len, int test_method);
#endif
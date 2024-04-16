#include <stdbool.h>
#include <stdint.h>

#ifndef SHARED_MEMORY_H_
#define SHARED_MEMORY_H_


void        set_up_L1_sets(uint64_t *s_evsetL1, uint64_t *s_evsetL1_c, uint64_t *s_evsetL1_m);
void        construct_L1_eviction_set(uint64_t* evset, uint64_t* page, uint64_t target, uint8_t SIZE, uint64_t STRIDE, uint64_t MAX_RAND);
uint64_t    get_new_random_index(uint64_t* rands, uint64_t MAX_RAND, int nb_so_far);
void        experimentation();
#endif
#include <stdbool.h>
#include <stdint.h>

#ifndef SHARED_MEMORY_H_
#define SHARED_MEMORY_H_


void        set_up_L1_sets(uint64_t *s_evsetL1, uint64_t *s_evsetL1_c, uint64_t *s_evsetL1_m);
void        experimentation();

void        configure_thresholds(uint64_t target_addr, int* thrL1, int* thrRAM, int* thrDET);
int         comp(const void * a, const void * b);
#endif
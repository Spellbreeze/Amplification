#ifndef CONFIGURATIONS_H_
#define CONFIGURATIONS_H_
#include <stdint.h>
#include "memory_utils.h"

#define ASSERT(x) assert(x != -1)

#define EVICT_L1_SIZE         ( 8*MB)
#define EVICT_L2_SIZE         ( 8*MB)
#define EVICT_LLC_SIZE        (128*MB)
#define SHARED_MEM_SIZE       (128*MB)

#define LLC_WAYS   16      // Number of L1-congruent addresses to find in preparation
#define NUMBER_CONGRUENT_L1   16      // Number of L1-congruent addresses to find in preparation
#define SMALLPAGE_PERIOD      (1 << 12)
#define MAX_POOL_SIZE_SMALL   (EVICT_LLC_SIZE/SMALLPAGE_PERIOD) 
#define MAX_POOL_SIZE         MAX_POOL_SIZE_SMALL

#define REPETITIONS (20000)
#define REFRESH (0)


#define PS_SUCCESS                1
#define PS_FAIL_CONSTRUCTION     -1
#define PS_FAIL_EXTENSION        -3
#define PS_FAIL_REDUCTION        -4
#define PS_FAIL_CONTDIR_EVICTION -5

#define EVTEST_MEAN     0
#define EVTEST_MEDIAN   1
#define EVTEST_ALLPASS  2

#define MAX_EXTENSION  32
#define MAX_ATTEMPT    20

#endif
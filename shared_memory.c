#include <stdbool.h>
#include <stdint.h>

#include "fr_util.h"
#include "amplify.h"
#include "memory_utils.h"
#include "shared_memory.h"

#include <assert.h>
#define ASSERT(x) assert(x != -1)

#define EVICT_L1_SIZE         ( 8*MB)
#define EVICT_L2_SIZE         ( 8*MB)
#define EVICT_LLC_SIZE        (128*MB)
#define SHARED_MEM_SIZE       (128*MB)

#define LLC_WAYS   16      // Number of L1-congruent addresses to find in preparation
#define NUMBER_CONGRUENT_L1   16      // Number of L1-congruent addresses to find in preparation
#define SMALLPAGE_PERIOD      (1 << 12)
#define MAX_POOL_SIZE_SMALL   (EVICT_LLC_SIZE/SMALLPAGE_PERIOD) 

#define REPETITIONS (10000)
#define REFRESH (0)

// This shared memory style is based on ShowTime

////////////////////////////////////////////////////////////////////////////////
// Memory Allocations
uint64_t *shared_mem, *evict_mem, *evict_mem2, *evict_mem_L1, *evict_mem_L1_m, *evict_mem_L1_c, *evict_mem_LLC, *evict_mem_LLC_c, *evict_mem_LLC2_c;
////////////////////////////////////////////////////////////////////////////////

uint64_t target_addr; uint64_t target_addr_m; uint64_t target_addr_m2;
int rand_target_index; int good_target_index; int target_index;

// Minimize the number of L1 cache sets that are polluted by the container arrays
uint64_t s_evsetL1[64*8];
uint64_t s_evsetL1_c[64*8];
uint64_t s_evsetL1_m[64*8];


int main(int argc, char **argv){
  // mmap some memory regions
  ASSERT(mem_map_private(&shared_mem, SHARED_MEM_SIZE));
  ASSERT(mem_map_private(&evict_mem, EVICT_LLC_SIZE));
  ASSERT(mem_map_private(&evict_mem2, EVICT_LLC_SIZE));
  ASSERT(mem_map_private(&evict_mem_L1, EVICT_LLC_SIZE));
  ASSERT(mem_map_private(&evict_mem_L1_c, EVICT_LLC_SIZE));
  ASSERT(mem_map_private(&evict_mem_L1_m, EVICT_LLC_SIZE));
  ASSERT(mem_map_private(&evict_mem_LLC, EVICT_LLC_SIZE));
  ASSERT(mem_map_private(&evict_mem_LLC_c, EVICT_LLC_SIZE));
  ASSERT(mem_map_private(&evict_mem_LLC2_c, EVICT_LLC_SIZE));

  // set up sets for eviction/traversal
  rand_target_index = (rand() & 0x3F)*8;
  good_target_index = (((uint64_t)s_evsetL1 & 0xFFF) >> 3);
  target_index = good_target_index+17*8;

  target_addr    = (uint64_t)&shared_mem[target_index];
  target_addr_m  = (uint64_t)&shared_mem[target_index+17*8];
  target_addr_m2 = (uint64_t)&shared_mem[target_index+20*8];

  set_up_L1_sets(s_evsetL1, s_evsetL1_c, s_evsetL1_m);

  //////////////////
    experimentation(); // Run tests
  //////////////////

  // ummap some memory regions
  ASSERT(munmap(shared_mem, SHARED_MEM_SIZE)); 
  ASSERT(munmap(evict_mem,  EVICT_LLC_SIZE));
  ASSERT(munmap(evict_mem2,  EVICT_LLC_SIZE));
  ASSERT(munmap(evict_mem_L1,  EVICT_LLC_SIZE));
  ASSERT(munmap(evict_mem_L1_c,  EVICT_LLC_SIZE));
  ASSERT(munmap(evict_mem_L1_m,  EVICT_LLC_SIZE));
  ASSERT(munmap(evict_mem_LLC,  EVICT_LLC_SIZE));
  ASSERT(munmap(evict_mem_LLC_c,  EVICT_LLC_SIZE));
  ASSERT(munmap(evict_mem_LLC2_c,  EVICT_LLC_SIZE));

  return 0;

}


//For Setting Up L1 Sets
void set_up_L1_sets(uint64_t *s_evsetL1, uint64_t *s_evsetL1_c, uint64_t *s_evsetL1_m) {
  construct_L1_eviction_set(s_evsetL1,   evict_mem_L1,   target_addr, NUMBER_CONGRUENT_L1, SMALLPAGE_PERIOD, MAX_POOL_SIZE_SMALL);
  construct_L1_eviction_set(s_evsetL1_c, evict_mem_L1_c, target_addr, NUMBER_CONGRUENT_L1, SMALLPAGE_PERIOD, MAX_POOL_SIZE_SMALL);
  construct_L1_eviction_set(s_evsetL1_m, evict_mem_L1_m, target_addr_m2, NUMBER_CONGRUENT_L1, SMALLPAGE_PERIOD, MAX_POOL_SIZE_SMALL);
}

void construct_L1_eviction_set(uint64_t* evset, uint64_t* page, uint64_t target, uint8_t SIZE, uint64_t STRIDE, uint64_t MAX_RAND) {
  uint64_t i, r;
  uint64_t rands[SIZE];
  
  for (i=0; i<SIZE; i++)  {

    r = get_new_random_index(rands, MAX_RAND, i);

    evset[i] = ((uint64_t)page + (target & (STRIDE-1)) + r*STRIDE);
  }
}

uint64_t get_new_random_index(uint64_t* rands, uint64_t MAX_RAND, int nb_so_far) {
  // Avoiding duplicates
  uint8_t goodRand = 0; int j; uint64_t r;

  while (!goodRand){
    goodRand = 1;
    r = rand() % MAX_RAND;
    for (j=0; j<nb_so_far; j++){
      if (rands[j] == r){ goodRand = 0; break; }
    }
  }

  rands[nb_so_far] = r;
  return r;
}

// For use in data collection
void sort(uint64_t array[], int sz) {
  for (int reps = 0; reps < sz; reps++) {
    for (int i = 0; i < sz-1; i++) {
      if (array[i] > array[i+1]) {
        uint64_t tmp = array[i];
        array[i] = array[i+1];
        array[i+1] = tmp;
      }
    }
  }
}

// Below is experimentation
void experimentation() {
  /*//TODO: implement eviction sets
  for (int i=0; i<LLC_WAYS; i++)                     { mwrite_v(evsetLLC[i], 0); }
  for (int i=0; i<LLC_WAYS; i++)                     { mwrite_v(evsetLLC_c[i], 0); }
  for (int i=0; i<LLC_WAYS; i++)                     { mwrite_v(evsetLLC2[i], 0); }
  for (int i=0; i<LLC_WAYS; i++)                     { mwrite_v(evsetLLC2_c[i], 0); }
  */
  for (int i=0; i<NUMBER_CONGRUENT_L1; i++)        { mwrite_v(s_evsetL1[i], 0); }
  for (int i=0; i<NUMBER_CONGRUENT_L1; i++)        { mwrite_v(s_evsetL1_c[i], 0); }
  for (int i=0; i<NUMBER_CONGRUENT_L1; i++)        { mwrite_v(s_evsetL1_m[i], 0); }
  mwrite_v(target_addr, 0);
  mwrite_v(target_addr_m, 0);

  // Pack all data values in a single L1 set to minimize pollution
    // Create local page-aligned array "fixed_array"
  uint64_t local_array[4096];
  uint64_t offset_for_set_0 = 0x1000 - ((uint64_t)local_array & 0xFFF);
  uint64_t *fixed_array = &local_array[offset_for_set_0/8];

  // Create offsets that will land in the same L1
  #define L1a_OFFSET (target_index+7*8+(0<<9))
  #define L1b_OFFSET (target_index+7*8+(1<<9))
  #define L1c_OFFSET (target_index+7*8+(2<<9))
  #define LLC_OFFSET (target_index+7*8+(3<<9))
  #define LLC2_OFFSET (target_index+7*8+(4<<9))
  #define VAR_OFFSET (target_index+7*8+(5<<9))

  #define L1a_LOCATION (&fixed_array[L1a_OFFSET])
  #define L1b_LOCATION (&fixed_array[L1b_OFFSET])
  #define L1c_LOCATION (&fixed_array[L1c_OFFSET])
  #define LLC_LOCATION (&fixed_array[LLC_OFFSET])
  #define LLC2_LOCATION (&fixed_array[LLC2_OFFSET])
  #define VAR_LOCATION (&fixed_array[VAR_OFFSET])

  // 8 pointers (8 byte each) fit in one cache line (64 bytes)
  for (int i = 0; i < 8; i++){ L1a_LOCATION[i] = s_evsetL1[i]; }
  for (int i = 0; i < 8; i++){ L1b_LOCATION[i] = s_evsetL1_c[i]; }
  for (int i = 0; i < 8; i++){ L1c_LOCATION[i] = s_evsetL1[8+i]; }
  //for (int i = 0; i < 8; i++){ LLC_LOCATION[i] = evsetLLC[i]; }
  //for (int i = 0; i < 8; i++){ LLC2_LOCATION[i] = evsetLLC[8+i]; }

  VAR_LOCATION[6] = target_addr_m;
  VAR_LOCATION[1] = 1;
  VAR_LOCATION[2] = 0;
  VAR_LOCATION[3] = target_addr_m2;
  VAR_LOCATION[4] = target_addr;
  VAR_LOCATION[5] = 0;
  VAR_LOCATION[0] = 0;
  //VAR_LOCATION[7] = REFRESH_RATE;


  // For data
  const int arrSize = 1000;
  uint64_t timings1[arrSize]; //noevict
  uint64_t timings2[arrSize]; //evict

  for (int dist=2; dist <= 7; dist++) {
    
    if (dist <= 4) {
      printf("***evict distance: %d***\n", dist);
      for (int i = 0; i < arrSize; i++) {
        timings1[i]    = leaky_run(s_evsetL1, s_evsetL1[0], s_evsetL1_c, REPETITIONS, dist, 0); //evictor and in_cache_evictor should be swapped
        timings2[i]    = leaky_run(s_evsetL1, s_evsetL1_c[0], s_evsetL1_c, REPETITIONS, dist, 0);
      }
    }
    else {
      printf("***reorder distance: %d***\n", dist-3);
      for (int i = 0; i < arrSize; i++) {
        timings1[i]    = showtime_reorder(s_evsetL1, 0, s_evsetL1_c, REPETITIONS, dist-3, 0);
        timings2[i]    = showtime_reorder(s_evsetL1, 1, s_evsetL1_c, REPETITIONS, dist-3, 0);
      }
    }

    sort(timings1, arrSize);
    sort(timings2, arrSize);

    uint64_t median1 = timings1[arrSize/2];
    uint64_t median2 = timings2[arrSize/2];

    puts("[*] no eviction");
    printf("      min: %zd\n", timings1[0]);
    printf("      max: %zd\n", timings1[arrSize-1]);
    printf("      median: %zd\n", median1);

    puts("[*] eviction");
    printf("      min: %zd\n", timings2[0]);
    printf("      max: %zd\n", timings2[arrSize-1]);
    printf("      median: %zd\n", median2);

    printf("[*] median diff: %zd (%zd%%)\n", median2 - median1, 100*(median2 - median1) / median1);
  }
}
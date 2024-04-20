#include <stdbool.h>
#include <stdint.h>

#include "fr_util.h"
#include "amplify.h"
#include "evset_generation.h"
#include "memory_utils.h"
#include "shared_memory.h"

#include <assert.h>
#include "configurations.h"


// This shared memory style is based on ShowTime

////////////////////////////////////////////////////////////////////////////////
// Memory Allocations
uint64_t *shared_mem, *evict_mem, *evict_mem2, *evict_mem_L1, *evict_mem_L1_m, *evict_mem_L1_c, *evict_mem_LLC, *evict_mem_LLC_c, *evict_mem_LLC2_c;
////////////////////////////////////////////////////////////////////////////////

uint64_t target_addr; uint64_t target_addr_m; uint64_t target_addr_m2;
int rand_target_index; int good_target_index; int target_index;
int thrL1, thrRAM, thrDET;

// Minimize the number of L1 cache sets that are polluted by the container arrays
uint64_t s_evsetL1[64*8];
uint64_t s_evsetL1_c[64*8];
uint64_t s_evsetL1_m[64*8];


void configure_thresholds(uint64_t target_addr, int* thrL1, int* thrRAM, int* thrDET) {

  #define THRESHOLD_TEST_COUNT 1000

  int timing[10][THRESHOLD_TEST_COUNT];
  int access_time;

  for (int t=0; t<THRESHOLD_TEST_COUNT; t++) {
    clflush(target_addr);
    timing[0][t] = maccess_t(target_addr); // time0: DRAM
    timing[1][t] = maccess_t(target_addr); // time1: L1/L2
  }
  qsort(timing[0], THRESHOLD_TEST_COUNT, sizeof(int), comp);
  qsort(timing[1], THRESHOLD_TEST_COUNT, sizeof(int), comp);
  *thrRAM = timing[0][(int)0.50*THRESHOLD_TEST_COUNT];
  *thrL1  = timing[1][(int)0.10*THRESHOLD_TEST_COUNT];
  *thrDET = (2*(*thrRAM) + (*thrL1))/3;
}

int comp(const void * a, const void * b) {
  return ( *(uint64_t*)a - *(uint64_t*)b );
}


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

  configure_thresholds(target_addr, &thrL1, &thrRAM, &thrDET);

  printf("\nThresholds Configured\n\n");
  printf("\tL1/L2    : %u\n", thrL1   );
  printf("\tRAM      : %u\n", thrRAM  );
  printf("\tTHRESHOLD: %u\n", thrDET  );
  // Remind the user how many LLC ways we are assuming
  printf("\nConfiguration: %d LLC ways\n", LLC_WAYS);

  // Construct L1 sets
  construct_L1_eviction_set(s_evsetL1,   evict_mem_L1,   target_addr, NUMBER_CONGRUENT_L1, SMALLPAGE_PERIOD, MAX_POOL_SIZE_SMALL);
  construct_L1_eviction_set(s_evsetL1_c, evict_mem_L1_c, target_addr, NUMBER_CONGRUENT_L1, SMALLPAGE_PERIOD, MAX_POOL_SIZE_SMALL);
  construct_L1_eviction_set(s_evsetL1_m, evict_mem_L1_m, target_addr_m2, NUMBER_CONGRUENT_L1, SMALLPAGE_PERIOD, MAX_POOL_SIZE_SMALL);

  // Construct LLC sets
  thrDET = 67;
  uint64_t evsetLLC[LLC_WAYS];    //generate_llc_eviction_set(target_addr,LLC_WAYS,evict_mem,thrDET,evsetLLC);
  uint64_t evsetLLC2[LLC_WAYS];   //generate_llc_eviction_set(target_addr_m2,LLC_WAYS,evict_mem2,thrDET,evsetLLC2);
  uint64_t evsetLLC_c[LLC_WAYS];  //generate_llc_eviction_set(target_addr,LLC_WAYS,evict_mem_LLC_c,thrDET,evsetLLC_c);
  uint64_t evsetLLC2_c[LLC_WAYS]; //generate_llc_eviction_set(target_addr_m2,LLC_WAYS,evict_mem_LLC2_c,thrDET,evsetLLC2_c);

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
  const int arrSize = 10000; // typically 1000
  uint64_t timings1[arrSize]; //noevict
  uint64_t timings2[arrSize]; //evict

  char *run_type[] = {"evict","reorder","simple","llc"};
  int repetitions[] = {1<<7,1<<8,1<<9,1<<10,1<<11, 1<<12, 1<<13, 1<<14,1<<15, 1<<16, 1<<17};

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j <= 11; j++) {
      for (int dist=2; dist <= 4; dist++) {
        if (i == 0) {
          // L1 Eviction Amplification
          printf("***%s distance: %d***\n", run_type[i], dist);
          for (int i = 0; i < arrSize; i++) {
            timings1[i]    = leaky_run(L1a_LOCATION, L1a_LOCATION[0], L1b_LOCATION, repetitions[j], dist, REFRESH); //evictor and in_cache_evictor should be swapped
            timings2[i]    = leaky_run(L1a_LOCATION, L1b_LOCATION[0], L1b_LOCATION, repetitions[j], dist, REFRESH);
          }
        }
        else if (i == 1) {
          // L1 Reordering Amplification
          printf("***%s distance: %d***\n", run_type[i], dist);
          for (int i = 0; i < arrSize; i++) {
            timings1[i]    = showtime_reorder(L1a_LOCATION, 0, L1b_LOCATION, repetitions[j], dist, REFRESH);
            timings2[i]    = showtime_reorder(L1a_LOCATION, 1, L1b_LOCATION, repetitions[j], dist, REFRESH);
          }
        }
        else if (i == 2) {
          // Simple Eviction Run (observed to have constant timing differences across all repetition amounts)
          if (dist != 2) {
            break;
          }
          timings1[i]    = leaky_run(L1a_LOCATION, L1a_LOCATION[0], L1b_LOCATION, repetitions[j], 1, REFRESH); //evictor and in_cache_evictor should be swapped
          timings2[i]    = leaky_run(L1a_LOCATION, L1b_LOCATION[0], L1b_LOCATION, repetitions[j], 1, REFRESH);
        }
        else {
          // LLC->L1 Amplification
          printf("***%s distance: %d***\n", run_type[i], dist);
          for (int i = 0; i < arrSize; i++) {
            timings1[i]    = showtime_llc_eviction(L1a_LOCATION, L1b_LOCATION, VAR_LOCATION[3], L1b_LOCATION[0], 0, repetitions[j], dist, REFRESH);
            timings2[i]    = showtime_llc_eviction(L1a_LOCATION, L1b_LOCATION, VAR_LOCATION[3], L1b_LOCATION[0], 1, repetitions[j], dist, REFRESH);
          }
        }

        sort(timings1, arrSize);
        sort(timings2, arrSize);

        uint64_t median1 = timings1[arrSize/2];
        uint64_t median2 = timings2[arrSize/2];

        printf("[*] no %s\n", run_type[i]);
        printf("      min: %zd\n", timings1[0]);
        printf("      max: %zd\n", timings1[arrSize-1]);
        printf("      median: %zd\n", median1);

        printf("[*] %s\n", run_type[i]);
        printf("      min: %zd\n", timings2[0]);
        printf("      max: %zd\n", timings2[arrSize-1]);
        printf("      median: %zd\n", median2);

        double median_diff = 0;
        if (median2 >= median1) {
          median_diff = 100*((double) (median2 - median1)) / median1;
          //printf("[*] median diff: %zd (%f%%)\n", median2 - median1, median_diff);
        }
        else {
          median_diff = 100*((double) (median1 - median2)) / median1;
          median_diff *= -1;
          //printf("[*] median diff: %zd (-%f%%)\n", median2 - median1, 100*((double) (median1 - median2)) / median1);
        }
        printf("[*] median diff: %zd (%f%%)\n", median2 - median1, median_diff);

        double quartile_diff = 0;
        if (timings2[1*(arrSize/4)] >= timings1[3*(arrSize/4)]) {
          quartile_diff = 100*((float)(timings2[1*(arrSize/4)] - timings1[3*(arrSize/4)])) / timings1[3*(arrSize/4)];
        }
        else {
          quartile_diff = 100*((float)(timings1[3*(arrSize/4)] - timings2[1*(arrSize/4)])) / timings1[3*(arrSize/4)];
          quartile_diff *= -1;
          //printf("[*] 3rd to 1st quartile diff: %zd (-%f%%)\n", timings2[1*(arrSize/4)] - timings1[3*(arrSize/4)], 100*((float)(timings1[3*(arrSize/4)] - timings2[1*(arrSize/4)])) / timings1[3*(arrSize/4)]);
        }
        printf("[*] 3rd to 1st quartile diff: %zd (%f%%)\n", timings2[1*(arrSize/4)] - timings1[3*(arrSize/4)], quartile_diff);

        FILE *fpt1; 
        char buf1[0x100];
        snprintf(buf1, sizeof(buf1), "data/timings/no_%s_dist%d.csv", run_type[i],dist);
        
        fpt1 = fopen(buf1, "a+");
        fprintf(fpt1,"%d,",repetitions[j]);
        for (int i = 0; i < arrSize; i++) {
          fprintf(fpt1,"%zd,",timings1[i]);
        }
        fprintf(fpt1,"\n");
        fclose(fpt1);

        FILE *fpt2;
        char buf2[0x100];
        snprintf(buf2, sizeof(buf2), "data/timings/%s_dist%d.csv", run_type[i],dist);
        fpt2 = fopen(buf2, "a+");
        fprintf(fpt2,"%d,",repetitions[j]);
        for (int i = 0; i < arrSize; i++) {
          fprintf(fpt2,"%zd,",timings2[i]);
        }
        fprintf(fpt2,"\n");
        fclose(fpt2);
        
        
        FILE *fpt3;
        char buf3[0x100];
        snprintf(buf3, sizeof(buf3), "data/medians/%s_dist%d.csv", run_type[i],dist);
        fpt3 = fopen(buf3, "a+");
        fprintf(fpt3,"%d,%zd,%zd,%zd,%f\n",repetitions[j],median2,median1,median2-median1, median_diff);
        fclose(fpt3);
        
        FILE *fpt4;
        char buf4[0x100];
        snprintf(buf4, sizeof(buf4), "data/quartile/%s_dist%d.csv", run_type[i],dist);
        fpt4 = fopen(buf4, "a+");
        fprintf(fpt4,"%d,%zd,%zd,%zd,%f\n",repetitions[j],timings2[1*(arrSize/4)],timings1[3*(arrSize/4)],timings2[1*(arrSize/4)]-timings1[3*(arrSize/4)], quartile_diff); 
        fclose(fpt4);//*/
      }
    }
  }
}
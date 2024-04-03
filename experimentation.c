#include "configurations.h"
#include "fr_util.h"
#include "amplify.h"
#include <stdint.h>

//**************************
// ADJUST THESE PARAMETERS IF NEEDED
//**************************
//Num Latency Buckets.
#define NUM_LAT_BUCKETS (8000) //TODO was 400
//Each Bucket Step.
#define BUCKET_LAT_STEP (1)
//Num Iterations
#define NUM_ITER        (1000)

#define BASE_FREQ_MHZ (3200)
#define GRANULARITY_USEC (1)

#define REPETITIONS (10000)
// Set maximum refresh rate
#define REFRESH (0)
//---------------------------------------------------------------------------

uint64_t* hits_lat_histogram ;
uint64_t* misses_lat_histogram ;
int num_hit_accesses = 0;
int num_miss_accesses = 0;

uint64_t sample_memory[1024*4096] __attribute__ ((aligned (4096))); 
ADDR_PTR addr;

uint64_t GRANULARITY_CYCLES = (uint64_t)BASE_FREQ_MHZ*(uint64_t)GRANULARITY_USEC;
//---------------------------------------------------------------------------

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

ADDR_PTR mem = (ADDR_PTR) 0x100000;

#define L1a_OFFSET (target_index+7*8+(0<<9))
  #define L1b_OFFSET (target_index+7*8+(1<<9))
  #define L1c_OFFSET (target_index+7*8+(2<<9))
  #define LLC_OFFSET (target_index+7*8+(3<<9))
  #define LLC2_OFFSET (target_index+7*8+(4<<9))
  #define VAR_OFFSET (target_index+7*8+(5<<9))
ADDR_PTR getRandomElement(ADDR_PTR mem_base) {
  ADDR_PTR pageIndex = rand() % 1000;
  return mem_base + pageIndex*0x1000 + 0x5c0;
}

int main(int argc, char **argv)
{
  for (int refresh = 0; refresh <= REFRESH; refresh <<= 2) {
    for (int j = 2; j <= 4; j++) {
      //Initialize the sample_memory
      srand(42);
      for (int i=0; i< 1024*4096; i++){
        sample_memory[i] = rand()%256;
      }
      //Initialize the cache set and addresses to test.
      ADDR_PTR clearSet[8];
      for (int i = 0; i < 8; i++) {
        clearSet[i] = getRandomElement((ADDR_PTR) &sample_memory[0]);//(ADDR_PTR) &sample_memory[(rand()% (4096))*1024];
        mwrite_v(clearSet[i], 0);
      }

      ADDR_PTR cacheSet[8];
      for (int i = 0; i < 8; i++) {
        cacheSet[i] = getRandomElement((ADDR_PTR) &sample_memory[0]);//(ADDR_PTR) &sample_memory[(rand()% (4096))*1024];
        mwrite_v(cacheSet[i], 0);
      }
      ADDR_PTR non_evictor =  cacheSet[0]; // in cache set
      
      ADDR_PTR evictor =  getRandomElement((ADDR_PTR) &sample_memory[0]);//(ADDR_PTR) &sample_memory[(rand()% (4096))*1024];
      mwrite_v(evictor, 0);   // not in cache set

      ADDR_PTR llc_candidate =  getRandomElement((ADDR_PTR) &sample_memory[0]);//(ADDR_PTR) &sample_memory[(rand()% (4096))*1024];
      mwrite_v(llc_candidate, 0);
      ADDR_PTR l1b_element =  getRandomElement((ADDR_PTR) &sample_memory[0]);//(ADDR_PTR) &sample_memory[(rand()% (4096))*1024];
      mwrite_v(l1b_element, 0);

      const int arrSize = 1000;
      uint64_t timings1[arrSize]; //noevict
      uint64_t timings2[arrSize]; //evict

      uint64_t timings3[arrSize]; //noreorder
      uint64_t timings4[arrSize];//reorder

      uint64_t timings5[arrSize]; //noflush
      uint64_t timings6[arrSize];//flush

      printf("{leaky.page} Distance %d\tRefresh %d:\n", j, refresh);
      for (int i = 0; i < arrSize; i++) {
        timings1[i] = leaky_run(cacheSet, non_evictor, clearSet, REPETITIONS, j, refresh); //evictor and in_cache_evictor should be swapped
        timings2[i]    = leaky_run(cacheSet, evictor, clearSet, REPETITIONS, j, refresh);
        
        timings3[i]    = showtime_reorder(cacheSet, 0, clearSet, REPETITIONS, j, refresh);
        timings4[i]    = showtime_reorder(cacheSet, 1, clearSet, REPETITIONS, j, refresh);
        
        timings5[i]    = showtime_llc_eviction(cacheSet, clearSet, llc_candidate, l1b_element, 0, REPETITIONS, j, refresh);
        timings6[i]    = showtime_llc_eviction(cacheSet, clearSet, llc_candidate, l1b_element, 1, REPETITIONS, j, refresh);
      }

      sort(timings1, arrSize);
      sort(timings2, arrSize);
      sort(timings3, arrSize);
      sort(timings4, arrSize);
      sort(timings5, arrSize);
      sort(timings6, arrSize);

      uint64_t median1 = timings1[arrSize/2];
      uint64_t median2 = timings2[arrSize/2];
      uint64_t median3 = timings3[arrSize/2];
      uint64_t median4 = timings4[arrSize/2];
      uint64_t median5 = timings5[arrSize/2];
      uint64_t median6 = timings6[arrSize/2];

      puts("[*] no eviction");
      printf("      min: %zd\n", timings1[0]);
      printf("      max: %zd\n", timings1[arrSize-1]);
      printf("      median: %zd\n", median1);

      puts("[*] eviction");
      printf("      min: %zd\n", timings2[0]);
      printf("      max: %zd\n", timings2[arrSize-1]);
      printf("      median: %zd\n", median2);

      printf("[*] median diff: %zd (%zd%%)\n", median2 - median1, 100*(median2 - median1) / median1);

      puts("[~] no reorder");
      printf("      min: %zd\n", timings3[0]);
      printf("      max: %zd\n", timings3[arrSize-1]);
      printf("      median: %zd\n", median3);

      puts("[~] reorder");
      printf("      min: %zd\n", timings4[0]);
      printf("      max: %zd\n", timings4[arrSize-1]);
      printf("      median: %zd\n", median4);

      printf("[*] median diff: %zd (%zd%%)\n", median4 - median3, 100*(median4 - median3) / median3);

      puts("[!] no flush");
      printf("      min: %zd\n", timings5[0]);
      printf("      max: %zd\n", timings5[arrSize-1]);
      printf("      median: %zd\n", median5);

      puts("[!] flush");
      printf("      min: %zd\n", timings6[0]);
      printf("      max: %zd\n", timings6[arrSize-1]);
      printf("      median: %zd\n", median6);

      printf("[*] median diff: %zd (%zd%%)\n", median6 - median5, 100*(median6 - median5) / median6);
    }

    if (refresh == 0) refresh = 1;
  }
}
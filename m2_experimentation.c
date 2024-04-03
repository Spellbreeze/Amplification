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

#define BASE_FREQ_MHZ (3000)
#define GRANULARITY_USEC (5)

#define REPETITIONS (10000)
// Set maximum refresh rate
#define REFRESH (4096)
//---------------------------------------------------------------------------

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
ADDR_PTR getRandomElement(ADDR_PTR mem_base) {
  ADDR_PTR pageIndex = rand() % 1000;
  return mem_base + pageIndex*0x1000 + 0x5c0;
}

int main(int argc, char **argv)
{
  const int arrSize = 10000;
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

      uint64_t timings1[arrSize]; //noevict

      for (int i = 0; i < arrSize; i++) {
        timings1[i]    = leaky_run(cacheSet, non_evictor, clearSet, REPETITIONS, j, refresh);
      }

      sort(timings1, arrSize);

      printf("No Eviction D=%d R=%d,", j, refresh);
      for (int i = 0; i < arrSize; i++) {
        printf("%zd,", timings1[i]);
      }

      printf("\n");
    }

    if (refresh == 0) refresh = 256;
  }

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

      uint64_t timings2[arrSize]; //evict

      for (int i = 0; i < arrSize; i++) {
        timings2[i]    = leaky_run(cacheSet, evictor, clearSet, REPETITIONS, j, refresh);
      }

      sort(timings2, arrSize);

      printf("Yes Eviction D=%d R=%d,", j, refresh);
      for (int i = 0; i < arrSize; i++) {
        printf("%zd,", timings2[i]);
      }

      printf("\n");
    }

    if (refresh == 0) refresh = 256;
  }

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

      uint64_t timings3[arrSize]; //noreorder

      for (int i = 0; i < arrSize; i++) {
        timings3[i]    = showtime_reorder(cacheSet, 0, clearSet, REPETITIONS, j, refresh);
      }

      sort(timings3, arrSize);

      printf("No Reorder D=%d R=%d,", j, refresh);
      for (int i = 0; i < arrSize; i++) {
        printf("%zd,", timings3[i]);
      }

      printf("\n");
    }

    if (refresh == 0) refresh = 256;
  }

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

      uint64_t timings4[arrSize];//reorder

      for (int i = 0; i < arrSize; i++) {
        timings4[i]    = showtime_reorder(cacheSet, 1, clearSet, REPETITIONS, j, refresh);
      }
      sort(timings4, arrSize);

      printf("Yes Reorder D=%d R=%d,", j, refresh);
      for (int i = 0; i < arrSize; i++) {
        printf("%zd,", timings4[i]);
      }

      printf("\n");
    }

    if (refresh == 0) refresh = 256;
  }
}
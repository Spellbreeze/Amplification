#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "configurations.h"
#include "list_utils.h"
#include "evset_generation.h"

#include "fr_util.h"

//For Setting Up L1 Sets

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

void generate_llc_eviction_set(uint64_t target_addr, int ways, uint64_t *evict_mem, int thrDET, uint64_t *evsetLLC) {
  Elem  *evsetList;  Elem **evsetList_ptr = &evsetList;
  *evsetList_ptr=NULL;

  int isEvsetSuccess = 0;
  do {
    isEvsetSuccess = ps_evset( evsetList_ptr, (char*)target_addr, ways, evict_mem, thrDET);
  } while (isEvsetSuccess != 1);

  printf("\nEviction set is constructed successfully");

  // Convert the eviction set link-list to an array
  list_to_array(evsetList, evsetLLC); //evsetLLC is updated
}


int ps_evset(Elem **evset, char *victim, int len, uint64_t* page, int threshold) {
  //int is_list_empty = 1; // append works the same for empty list, no special treatment needed
  int list_len = 0;
  int len_requested = len;
  int guess_index=0;
  int counter_attempt = 0;
  int i;
  int counter_guess, try_guesses, time;

  Elem *evset_last = NULL;
  *evset = NULL;
  
  //////////////////////////////////////////////////////////////////////////////
  // Create a guess pool
  uint64_t guess_pool[MAX_POOL_SIZE];
  int pool_size = MAX_POOL_SIZE;

  /*
  for (i=0; i<pool_size; i++)  {
    guess_pool[i] =
      ((uint64_t)page + ((uint64_t)victim & (SMALLPAGE_PERIOD-1)) + i*SMALLPAGE_PERIOD);
  }
  //*/
  
  // Potential improvement: randomization could be better (e.g., to avoid duplicates)
  for (i=0; i<pool_size; i++)  {
    guess_pool[i] =
      ((uint64_t)page + ((uint64_t)victim & (SMALLPAGE_PERIOD-1)) + (rand() % MAX_POOL_SIZE_SMALL)*SMALLPAGE_PERIOD);
  }
  //*/

  //////////////////////////////////////////////////////////////////////////////
  // Start finding eviction set
extend:
  while (list_len<len) {
    counter_guess = 0;
    try_guesses = true;

    // Place TARGET in LLC
    maccess((uint64_t)victim);
    asm volatile("lfence;mfence");

    //#if ENABLE_ALREADY_FOUND
    //// Place ALREADY_FOUND in LLC
    //traverse_zigzag_victim(*evset, (void*)victim); 
    //traverse_zigzag_victim(*evset, (void*)victim); 
    //#endif

    // Search 
    while (try_guesses) {
      // Access guess
      maccess((uint64_t)guess_pool[guess_index]);
      asm volatile("lfence");

      // Measure TARGET
      time = maccess_t((uint64_t)victim);
    
      // If TARGET is evicted
      //if (time>threshold-20 && time < 2*threshold) { 
      if (time>threshold-20) { 
        try_guesses = false;
        counter_attempt = 0;

        // Add the guess to linkedlist
        evset_last = (Elem*)guess_pool[guess_index];
        list_len = list_append(evset, evset_last);
        
        // Potential improvement: linked list for easy removal
        for (int i=guess_index; i<pool_size-1; i++){
          guess_pool[i] = guess_pool[i+1];
        }
        pool_size--;
      }
      else {
        guess_index++;
      }

      // Wrap around the pool
      if (guess_index>=pool_size-1) {\
        guess_index = 0;
        try_guesses = false; // reinstate victim to be sure
        if (++counter_attempt>=MAX_ATTEMPT){ // If too many wrap-arounds, return with fail
          printf("p");
          return PS_FAIL_CONSTRUCTION;
        }
      }
    }
  }
    //#if ENABLE_EXTENSION 
    // If list of minimal size cannot evict victim, extend it
    if (!ps_evset_test(evset, victim, threshold, 10, EVTEST_MEDIAN)) { 
      if (++len<MAX_EXTENSION)
        goto extend; // Obtain one more address
      //printf("e");
      return PS_FAIL_EXTENSION;
    }
    //#endif

    //#if ENABLE_REDUCTION
    // If list needed to be extended, reduce it to minimal
    if (list_len > len_requested){
      if (!ps_evset_reduce(evset, victim, len_requested, threshold)){
        //printf("r");
        return PS_FAIL_REDUCTION;
      }
    }
    //#endif//*/

  //printf("\n\tSucceeded Guess: %d\n", counter_attempt);
  return PS_SUCCESS;
}

int ps_evset_reduce(Elem **evset, char *victim, int len, int threshold) {
  int list_len = list_length(*evset), i;

  for (i=0; i<list_len; i++) {
    // Pop the first element
    Elem* popped = list_pop(evset);

    // If the reduced list evicts the TARGET, popped element can be removed
    if (ps_evset_test(evset, victim, threshold, 10, EVTEST_MEDIAN)) {
      if (list_length(*evset) == len){ // If the reduced list is minimal, SUCCESS
        return 1; // SUCCESS
      }
    }
    else { // If not, append the popped element to the end of list, and try again
      list_append(evset, popped); 
    }     
  }

  return 0; // FAIL
}

////////////////////////////////////////////////////////////////////////////////
static 
inline 
int 
comp(const void * a, const void * b) {
  return ( *(uint64_t*)a - *(uint64_t*)b );
}

inline
void
traverse_list_simple(Elem *ptr)
{
	while (ptr)
	{
		maccess ((uint64_t)ptr);
		ptr = ptr->next;
	}
}

int ps_evset_test(Elem **evset, char *victim, int threshold, int test_len, int test_method) {
  // Check, whether the reduced list can evict the TARGET 
    
  int test, time[test_len], time_acc=0;
  int i=0;

  // Place TARGET in LLC
  asm volatile("lfence;mfence");
  maccess((uint64_t)victim);
  asm volatile("lfence;mfence");

  for (test=0; test<test_len; test++) {

    // Potential improvement: could be sped up
    traverse_list_simple(*evset);//traverse_list_asm_skylake(*evset);
    traverse_list_simple(*evset);//traverse_list_asm_skylake(*evset);
    traverse_list_simple(*evset);//traverse_list_asm_skylake(*evset);
    traverse_list_simple(*evset);//traverse_list_asm_skylake(*evset);
    asm volatile("lfence;mfence");

    // Measure TARGET (and place in LLC for next iteration)
    time[test] = maccess_t((uint64_t)victim);
    time_acc += time[test];
  }

  if (test_method == EVTEST_MEAN) {

    return (time_acc/test_len)>threshold;
  
  } else if (test_method == EVTEST_MEDIAN) {

    qsort(time, test_len, sizeof(int), comp);

    /*printf("Test Times: ");
    for (int i = 0; i < test_len; i++) {
      printf("%d, ", time[i]);
    }
    printf("vs. Threshold=%d\n",threshold);*/
      
    return time[test_len/2]>threshold;
  
  } else if (test_method == EVTEST_ALLPASS) {

    int all_passed = 1;
    for (test=0; test<test_len; test++)
      if (all_passed && time[test]<threshold) 
        all_passed = 0;
      
    return all_passed;
  }
  else {
    return 0;
  }
}

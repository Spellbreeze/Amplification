#include <stdbool.h>
#include <stdint.h>

#include "fr_util.h"
#include "amplify.h"


// LEAKY.PAGE-style cache eviction amplification (with ShowTime variations and refresh)
void map_or_die(ADDR_PTR addr, size_t sz) {
  if (mmap((char *) addr, sz, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0) != (char *) addr) {
    err(1, "mmap");
  }
}

uint64_t prepare_PLRU_state(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash) {
  trash = maccess(clearSet[0]+trash);
  trash = maccess(clearSet[1]+trash);
  trash = maccess(clearSet[2]+trash);
  trash = maccess(clearSet[3]+trash);
  trash = maccess(clearSet[4]+trash);
  trash = maccess(clearSet[5]+trash);
  trash = maccess(clearSet[6]+trash);
  trash = maccess(clearSet[7]+trash);

  trash = maccess(cacheSet[0]+trash);
  trash = maccess(cacheSet[1]+trash);
  trash = maccess(cacheSet[2]+trash);
  trash = maccess(cacheSet[3]+trash);
  trash = maccess(cacheSet[4]+trash);
  trash = maccess(cacheSet[5]+trash);
  trash = maccess(cacheSet[6]+trash);
  trash = maccess(cacheSet[7]+trash);
  return trash;
}

//(ACDEFGH)_b4 traversal
uint64_t leaky_run(ADDR_PTR *cacheSet, ADDR_PTR evict, ADDR_PTR *clearSet, int repetitions, int dist, int refresh) {
  ADDR_PTR trash = 0;
  for (volatile int i = 0; i < 2048; i++);

  trash = prepare_PLRU_state(cacheSet, clearSet, trash);

  trash = maccess(evict+trash);

  uint64_t cycles_taken = 0;
  if (dist == 4) {
    cycles_taken = leaky_run_b4(cacheSet, clearSet, trash, repetitions, refresh);
  }
  else if (dist == 3) {
    cycles_taken = leaky_run_b3(cacheSet, clearSet, trash, repetitions, refresh);
  }
  else if (dist == 2) {
    cycles_taken = leaky_run_b2(cacheSet, clearSet, trash, repetitions, refresh);
  }
  else {
    cycles_taken = simple_run(cacheSet,clearSet,trash,repetitions,refresh);
  }
  return cycles_taken;
}

//(ACDEFGH)_b4 traversal
uint64_t leaky_run_b4(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh) {
  uint64_t start = rdtsc_begin();
  for (int rep = 0; rep < repetitions; rep++) {
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[1]+trash);
    trash = maccess(cacheSet[2]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[3]+trash);
    trash = maccess(cacheSet[5]+trash);
    trash = maccess(cacheSet[6]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[7]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[1]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[2]+trash);
    trash = maccess(cacheSet[3]+trash);
    trash = maccess(cacheSet[5]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[6]+trash);
    trash = maccess(cacheSet[7]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[1]+trash);
    trash = maccess(cacheSet[2]+trash);
    trash = maccess(cacheSet[3]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[5]+trash);
    trash = maccess(cacheSet[6]+trash);
    trash = maccess(cacheSet[7]+trash);
    trash = maccess(cacheSet[4]+trash);
    
    if (refresh && !(refresh > repetitions) && !(refresh & rep)) {
      trash = leaky_refresh_b4(cacheSet, clearSet, trash);
    }
  }
  uint64_t end = rdtsc_end();
  return end - start;
}

//(ACDEFGH)_b3 traversal
uint64_t leaky_run_b3(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh) {
  uint64_t start = rdtsc_begin();
  for (int rep = 0; rep < repetitions; rep++) {
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[1]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[2]+trash);
    trash = maccess(cacheSet[3]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[5]+trash);
    trash = maccess(cacheSet[6]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[7]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[1]+trash);
    trash = maccess(cacheSet[2]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[3]+trash);
    trash = maccess(cacheSet[5]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[6]+trash);
    trash = maccess(cacheSet[7]+trash);
    trash = maccess(cacheSet[4]+trash);
    
    if (refresh && !(refresh > repetitions) && !(refresh & rep)) {
      trash = leaky_refresh_b4(cacheSet, clearSet, trash);
    }
  }
  uint64_t end = rdtsc_end();
  return end - start;
}

//(ACDEFGH)_b2 traversal
uint64_t leaky_run_b2(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh) {
  uint64_t start = rdtsc_begin();
  for (int rep = 0; rep < repetitions; rep++) {
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[1]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[2]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[3]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[5]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[6]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[7]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[1]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[2]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[3]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[5]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[6]+trash);
    trash = maccess(cacheSet[4]+trash);

    trash = maccess(cacheSet[7]+trash);
    trash = maccess(cacheSet[4]+trash);
    
    if (refresh && !(refresh > repetitions) && !(refresh & rep)) {
      trash = leaky_refresh_b4(cacheSet, clearSet, trash);
    }
  }
  uint64_t end = rdtsc_end();
  return end - start;
}

uint64_t simple_run(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash, int repetitions, int refresh) {
  uint64_t start = rdtsc_begin();
  for (int rep = 0; rep < repetitions; rep++) {
    
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);

    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);

    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);

    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    trash = maccess(cacheSet[0]+trash);
    //*/

    if (refresh && !(refresh > repetitions) && !(refresh & rep)) {
      trash = leaky_refresh_b4(cacheSet, clearSet, trash);
    }
  }
  uint64_t end = rdtsc_end();
  return (end - start);
}


uint64_t leaky_refresh_b4(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash) {
  trash = maccess(clearSet[0]+trash);
  trash = maccess(clearSet[1]+trash);
  trash = maccess(clearSet[2]+trash);
  trash = maccess(cacheSet[4]+trash);//AB//ABC
  
  trash = maccess(clearSet[3]+trash);
  trash = maccess(clearSet[4]+trash);
  trash = maccess(clearSet[5]+trash);
  trash = maccess(cacheSet[4]+trash);//DEF
  
  trash = maccess(cacheSet[1]+trash);
  trash = maccess(cacheSet[2]+trash);
  trash = maccess(cacheSet[3]+trash);
  trash = maccess(cacheSet[4]+trash);//123
  
  
  trash = maccess(cacheSet[5]+trash);
  trash = maccess(cacheSet[6]+trash);
  trash = maccess(cacheSet[7]+trash);
  trash = maccess(cacheSet[4]+trash);//456
  return trash;
}

uint64_t leaky_refresh_b3(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash) {
  trash = maccess(clearSet[0]+trash);
  trash = maccess(clearSet[1]+trash);
  trash = maccess(cacheSet[4]+trash);//AB

  trash = maccess(clearSet[2]+trash);
  trash = maccess(clearSet[3]+trash);
  trash = maccess(cacheSet[4]+trash);//CD

  trash = maccess(clearSet[4]+trash);
  trash = maccess(cacheSet[4]+trash);//E

  trash = maccess(clearSet[5]+trash);
  trash = maccess(cacheSet[4]+trash);//F

  trash = maccess(cacheSet[1]+trash);
  trash = maccess(cacheSet[2]+trash);
  trash = maccess(cacheSet[4]+trash);//12

  trash = maccess(cacheSet[3]+trash);
  trash = maccess(cacheSet[6]+trash);
  trash = maccess(cacheSet[4]+trash);//35 (b takes [4], so 5-->6)

  trash = maccess(cacheSet[5]+trash);
  trash = maccess(cacheSet[4]+trash);//4 (b takes [4], so 4-->5)

  trash = maccess(cacheSet[7]+trash);
  trash = maccess(cacheSet[4]+trash);//6 (b takes [4], so 6-->7)
  return trash;
}

uint64_t leaky_refresh_b2(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR trash) {
  trash = maccess(clearSet[0]+trash); trash = maccess(cacheSet[4]+trash);//A
  trash = maccess(clearSet[1]+trash); trash = maccess(cacheSet[4]+trash);//B
  trash = maccess(clearSet[2]+trash); trash = maccess(cacheSet[4]+trash);//C
  trash = maccess(clearSet[3]+trash); trash = maccess(cacheSet[4]+trash);//D
  trash = maccess(cacheSet[1]+trash); trash = maccess(cacheSet[4]+trash);//1
  trash = maccess(cacheSet[3]+trash); trash = maccess(cacheSet[4]+trash);//3
  trash = maccess(cacheSet[5]+trash); trash = maccess(cacheSet[4]+trash);//4 (b takes [4], so 4-->5)
  trash = maccess(cacheSet[7]+trash); trash = maccess(cacheSet[4]+trash);//6 (b takes [4], so 6-->7)
  return trash;
}


// Reordering Amplification:
uint64_t showtime_reorder(ADDR_PTR *cacheSet, int reorder, ADDR_PTR *clearSet, int repetitions, int dist, int refresh) {
  ADDR_PTR trash = 0;
  for (volatile int i = 0; i < 2048; i++);

  // Prepare L1 PLRU state
  trash = prepare_PLRU_state(cacheSet, clearSet, trash);

  if (reorder) {
    lmfence();
    trash = maccess(clearSet[0]+trash);
    trash = maccess(cacheSet[4]+trash);
    lmfence();
  }
  else {
    lmfence();
    trash = maccess(cacheSet[4]+trash);
    trash = maccess(clearSet[0]+trash);
    lmfence();
  }

  //Adaptor Pattern FHB
  trash = maccess(cacheSet[5]+trash);
  trash = maccess(cacheSet[7]+trash);
  trash = maccess(cacheSet[4]+trash);

  uint64_t cycles_taken = 0;
  if (dist == 4) {
    cycles_taken = leaky_run_b4(cacheSet, clearSet, trash, repetitions, refresh);
  }
  else if (dist == 3) {
    cycles_taken = leaky_run_b3(cacheSet, clearSet, trash, repetitions, refresh);
  }
  else {
    cycles_taken = leaky_run_b2(cacheSet, clearSet, trash, repetitions, refresh);
  }
  return cycles_taken;
}


// Showtime LLC Eviction
uint64_t showtime_llc_eviction(ADDR_PTR *cacheSet, ADDR_PTR *clearSet, ADDR_PTR llc_candidate, ADDR_PTR l1b_element, int flushed, int repetitions, int dist, int refresh) {
  ADDR_PTR trash = 0, z = 0, j3 = 0, j4 = 0;
  for (volatile int i = 0; i < 2048; i++);

  // Prepare LLC
  trash = maccess(llc_candidate + trash);
  if (flushed) {
    lmfence();
    clflush(llc_candidate);
    lmfence();
  }

  // Prepare L1 PLRU state
  trash = prepare_PLRU_state(cacheSet, clearSet, trash);

  // Convert LLC presence to L1
  j3 = maccess(llc_candidate + trash);
  j3 = maccess(cacheSet[6] + j3); // D == [6]
  for (z = 17^trash; z > 0; z--) {}
  j4 = maccess(cacheSet[7]^z);
  trash = maccess(l1b_element^j4^j3); 

  //Adaptor Pattern FHB
  trash = maccess(cacheSet[5]+trash);
  trash = maccess(cacheSet[7]+trash);
  trash = maccess(cacheSet[4]+trash);

  //Run conversion on normal PLRU traversal
  uint64_t cycles_taken = 0;
  if (dist == 4) {
    cycles_taken = leaky_run_b4(cacheSet, clearSet, trash, repetitions, refresh);
  }
  else if (dist == 3) {
    cycles_taken = leaky_run_b3(cacheSet, clearSet, trash, repetitions, refresh);
  }
  else {
    cycles_taken = leaky_run_b2(cacheSet, clearSet, trash, repetitions, refresh);
  }
  return cycles_taken;
}
//*/

// Prefetch Eviction
/*
  j0=0;
  WRITE_VALUE(M_A,0);
  WRITE_VALUE(M_B,0);
  for (int it=0; it<LLC_WAYS; it++){ WRITE_VALUE(evsetLLC2[it],0); }
  for (int it=2; it<LLC_WAYS; it++){ WRITE_VALUE(evsetLLC2_c[it],0); }
  for (ITERATOR = 0; ITERATOR < AMP_ITERATIONS; ITERATOR++){

    for (int it=0; it<LLC_WAYS; it++){ FLUSH(evsetLLC2[it]); LMFENCE;}
    for (int it=0; it<LLC_WAYS; it++){ FLUSH(evsetLLC2_c[it]); LMFENCE;}
    for (int it=0; it<LLC_WAYS; it++){ TOUCH(evsetLLC2[it]); LMFENCE;}
    
    if (ITERATOR & 0x1){ LMFENCE; READ(M_A); } LMFENCE; // Access on odd iterations

    LLC_PREFETCH[ITERATOR] = MEASURE_PREFETCH(M_A, M_B); // Traverse pattern
  }
*/
/*uint64_t showtime_prefetch_eviction() {
  ADDR_PTR trash = 0;
  // 1) Construct (different) two eviction sets (make sure that all addresses point to 0 values)

  // 2) Note two addresses (A & B) from one of the eviction sets (E1)

  // 3) Iterate:

    // i) Flush both eviction sets from the caches

    // ii) Ensure all of the other eviction set (that does not contain A or B) is in the LLC (read to each address)

    // iii) Enter or don't enter A into the cache with a read, depending on experiment

    // iv) Traverse through the prefetch pattern and return the timing.
}*/

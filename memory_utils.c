#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "memory_utils.h"


//Included from ShowTime
int mem_map_private(uint64_t** addr, int len) {
  *addr = (uint64_t*) mmap (NULL, len, PROT_READ|PROT_WRITE, PFLAGS_4K, 0, 0);

  if (*addr == MAP_FAILED) {
    if (errno == ENOMEM) {
        printf("Could not allocate buffer (out of memory)\n");
    }
    printf("mmap failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int mem_map_shared(uint64_t** addr, int len) {
  // map shared memory to process address space
  *addr = (uint64_t*) mmap (NULL, len, SPROTECTION, SFLAGS_4K, 0, 0);
  
  if (*addr == MAP_FAILED) {
    if (errno == ENOMEM) {
        printf("Could not allocate buffer (out of memory)\n");
    }
    printf("mmap failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int mem_unmap(uint64_t *addr, int len) {
  
  if (len > 2 * MB)
    len = (len + (1 * GB - 1)) & (~(1 * GB - 1));
  else if (len > 4 * KB)
    len = 2 * MB;

  // un-map
  if (munmap((void*)addr, len) == -1) {
    printf("munmap failed\n");
    return -1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int var_map_shared(volatile uint64_t** addr) {
  int len = sizeof *addr;
  
  *addr = (volatile uint64_t*)mmap(NULL, len, SPROTECTION, SFLAGS_4K, 0, 0);
  
  if (*addr == MAP_FAILED) {
    printf("mmap failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int var_unmap(volatile uint64_t *addr) {
  int len = sizeof *addr;
  
  // un-map
  if (munmap((void*)addr, len) == -1) {
    printf("munmap failed\n");
    return -1;
  }
  return 0;
}

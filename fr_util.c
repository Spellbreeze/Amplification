#include "fr_util.h"
#include <stdbool.h>
#include <stdint.h>


//----------------------------------------------------------------
// PRIMITIVES FOR FLUSH+RELOAD CHANNEL
//----------------------------------------------------------------

/* Flush a cache block of address "addr" */
extern inline __attribute__((always_inline))
void clflush(ADDR_PTR addr)
{
  //TODO: Use clflush instruction.
  asm volatile (
    /*"mfence\n"*/
    "clflush (%0)"
		: /*output*/
		: /*input*/ "r"(addr)
		: /*clobbers*/ );
}


/* Load address "addr" */
ADDR_PTR maccess(ADDR_PTR addr)
{
  ADDR_PTR tmp;
  //TODO: Use mov instruction.
  asm volatile("mov (%1), %0"
	       : /*output*/ "=r" (tmp)
	       : /*input*/ "r"(addr)
	       : /*clobbers*/  );
  
  return tmp;
}

/* Loads addr and measure the access time */
CYCLES maccess_t(ADDR_PTR addr)
{
  CYCLES cycles;
  uint64_t tmp;
  // TODO:
  // Use a mov instruction to load an address "addr",
  // which is sandwiched between two rdtscp instructions.
  // Calculate the latency using difference of the output of two rdtscps.
  asm volatile(
    "mfence\n"
    "mov %2, %%rbx\n"
    "rdtscp\n"
    "shl $32,%%rdx\n"
		"or %%rax, %%rdx\n"
    "mov %%rbx, %%rax\n"
    "mov %%rdx, %%rbx\n"
    "mov (%%rax), %1\n"
    "rdtscp\n"
    "mfence\n"
    "shl $32,%%rdx\n"
		"or %%rax, %%rdx\n"
    "sub %%rbx, %%rdx"
	       : /*output*/ "=d"(cycles), "=r"(tmp)
	       : /*input*/  "r"(addr)
	       : /*clobbers*/ "rax", "rbx", "rcx");    
  return cycles;
}

inline 
void 
mwrite_v(ADDR_PTR addr, uint64_t value)
{
  asm volatile (
    "mfence\n\t"
    "lfence\n\t"
    "mov %1, (%0)\n\t"
    "mfence\n\t"
    : 
    : "r" (addr), "ri" (value)
    : );
}

/* Returns Time Stamp Counter (using rdtscp function)*/
extern inline __attribute__((always_inline))
uint64_t rdtscp(void) {
  uint64_t cycles;
  asm volatile ("rdtscp\n"
		"shl $32,%%rdx\n"
		"or %%rdx, %%rax\n"		      
		: /* outputs */ "=a" (cycles));
  return cycles;
}

extern inline __attribute__((always_inline))
uint64_t rdtsc_begin() {
  uint32_t high, low;
  asm volatile (
      "CPUID\n\t"
      "RDTSC\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t"
      : "=r" (high), "=r" (low)
      :
      : "rax", "rbx", "rcx", "rdx");
  return ((uint64_t)high << 32) | low;
}

extern inline __attribute__((always_inline))
uint64_t rdtsc_end() {
  uint32_t high, low;
  asm volatile(
      "RDTSCP\n\t"
      "mov %%edx, %0\n\t"
      "mov %%eax, %1\n\t"
      "CPUID\n\t"
      : "=r" (high), "=r" (low)
      :
      : "rax", "rbx", "rcx", "rdx");
  return ((uint64_t)high << 32) | low;
}

inline
uint64_t
// modified from https://github.com/cgvwzq/evsets/blob/master/micro.h 
rdtsc_cripple(uint64_t cycle_granularity)
{
	unsigned a, d;
	__asm__ volatile ("lfence\n"
	"rdtscp\n"
	"mov %%edx, %0\n"
	"mov %%eax, %1\n"
	: "=r" (a), "=r" (d)
	:: "%rax", "%rbx", "%rcx", "%rdx");
	return (uint64_t) ((uint64_t) ((uint64_t) ((uint64_t)a << 32)) | d) / cycle_granularity;
}

inline
void lmfence() {
  asm volatile("lfence\nmfence\n");
  return;
}

inline
void mfence() {
  asm volatile("mfence\n");
  return;
}

inline
void lfence() {
  asm volatile("lfence\n"); 
  return;
}

/* Synchronize sender and receiver for each bit 
   We will use a counter that counts up to SYNC_TIME_MASK)
 * -- Create the counter by masking lower bits of rdtscp using SYNC_TIME_MASK
 * -- Spin until the counter overflows and becomes less than SYNC_JITTER
 */
extern inline __attribute__((always_inline))
CYCLES cc_sync(uint64_t SYNC_TIME_MASK, uint64_t SYNC_JITTER) {
  CYCLES end_sync_cycle; //cycle where synchronization is done.
  //TODO:
  //Get the current counter value by masking lower bits of rdtscp using SYNC_TIME_MASK
  CYCLES current_counter = rdtscp();
  //Spin until current counter value overflows and is within SYNC_JITTER.
  //bool hasOverflowed = false;
  while ((current_counter & SYNC_TIME_MASK) > SYNC_JITTER) {
    current_counter = rdtscp();
    /*// TODO: does this work for being within SYNC_JITTER?
    if (!hasOverflowed && (current_counter & SYNC_TIME_MASK) < SYNC_JITTER) {
      hasOverflowed = true;
    }//*/
  }
  return current_counter;
}


//----------------------------------------------------------------
// NO NEED TO CHANGE ANYTHING BELOW THIS LINE
//----------------------------------------------------------------

/*
 * Convert a given ASCII string to a binary string.
 * From:
 * https://stackoverflow.com/questions/41384262/convert-string-to-binary-in-c
 */
char *string_to_binary(char *s)
{
    if (s == NULL) return 0; /* no input string */
    size_t len = strlen(s) ;

    // Each char is one byte (8 bits) and + 1 at the end for null terminator
    char *binary = malloc(len * 8 + 1);
    binary[0] = '\0';
	
    for (size_t i = 0; i < len; ++i) {
        char ch = s[i];
        for (int j = 7; j >= 0; --j) {
            if (ch & (1 << j)) {
                strcat(binary, "1");
            } else {
                strcat(binary, "0");
            }
        }
    }    
    return binary;
}

/*
 * Convert 8 bit data stream into character and return
 */
char *conv_char(char *data, int size, char *msg)
{
    for (int i = 0; i < size; i++) {
        char tmp[8];
        int k = 0;

        for (int j = i * 8; j < ((i + 1) * 8); j++) {
            tmp[k++] = data[j];
        }

        char tm = strtol(tmp, 0, 2);
        msg[i] = tm;
    }

    msg[size] = '\0';
    return msg;
}

/*
 * Prints help menu
 */
void print_help() {

	printf("-f,\tFile to be shared between sender/receiver\n"
		"-o,\tSelected offset into shared file\n"
		"-s,\tTime period on which sender and receiver sync on each bit\n"
	       	"-i,\tTime interval for sending a single bit within a sync period\n");

}

/*
 * Parses the arguments and flags of the program and initializes the struct config
 * with those parameters (or the default ones if no custom flags are given).
 */
void init_config(struct config *config, int argc, char **argv)
{
  // Initialize default config parameters
  int offset = DEFAULT_FILE_OFFSET;
  config->tx_interval      = TX_INTERVAL_DEF;
  config->sync_time_mask   = SYNC_TIME_MASK_DEF;
  config->sync_jitter      = SYNC_JITTER_DEF;
    
  char *filename = DEFAULT_FILE_NAME;


  // Parse the command line flags
  //      -f is used to specify the shared file 
  //      -o is used to specify the shared file offset
  //      -s is used to specify the sync_time mask
  //      -i is used to specify the sending interval rate
  
  int option;
  while ((option = getopt(argc, argv, "i:s:o:f:")) != -1) {
    switch (option) {
    case 'i':
      config->tx_interval     = atoi(optarg);
      break;
    case 's':
      config->sync_time_mask = atoi(optarg);
      break;        
    case 'o':
      offset = atoi(optarg)*CACHE_BLOCK_SIZE;
      break;
    case 'f':
      filename = optarg;
      break;
    case 'h':
      print_help();
      exit(1);
    case '?':
      fprintf(stderr, "Unknown option character\n");
      print_help();
      exit(1);
    default:
        print_help();
        exit(1);
    }
  }

  // Map file to virtual memory and extract the address at the file offset
  if (filename != NULL) {
    int inFile = open(filename, O_RDONLY);
    if(inFile == -1) {
      printf("Failed to Open File\n");
      exit(1);
    }

    void *mapaddr = mmap(NULL,DEFAULT_FILE_SIZE,PROT_READ,MAP_SHARED,inFile,0);

    if (mapaddr == (void*) -1 ) {
      printf("Failed to Map Address\n");
      exit(1);
    }

    config->addr = (ADDR_PTR) mapaddr + offset;
  }
}


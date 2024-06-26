CC=gcc
# CFLAGS=-std=gnu99 #Uncomment if using GCC versions older than 5 (where it may otherwise throw some errors)
CFLAGS=-O3 -g

all: clean experimentation m2_experimentation shared

shared: shared_memory.o fr_util.o amplify.o memory_utils.o evset_generation.o list_utils.o
	$(CC) $(CFLAGS) $^ -o $@

experimentation: experimentation.o fr_util.o amplify.o
	$(CC) $(CFLAGS) $^ -o $@

m2_experimentation: m2_experimentation.o fr_util.o amplify.o
	$(CC) $(CFLAGS) $^ -o $@

fr_util.o: fr_util.c fr_util.h
	$(CC) $(CFLAGS) -c $<

amplify.o: amplify.c amplify.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c fr_util.h configurations.h amplify.h
	$(CC) $(CFLAGS)  -c $< 

clean:
	rm -f *.o *~ amplify fr_util experimentation m2_experimentation shared

data_clean:
	rm -rf data/medians/*.csv data/quartile/*.csv data/timings/*.csv
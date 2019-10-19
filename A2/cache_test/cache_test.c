
#define _GNU_SOURCE

#include <sched.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define NUMBER_OF_RUNS 30

int main(int argc, char* argv[])
{
    unsigned int core = 0;
    unsigned int size_in_kb = 0;

    FILE* output_file = NULL;

    int op = 0;
    while((op = getopt(argc, argv, "c:k:f:")) != -1)
    {
        switch(op)
        {
            case 'c':
            {
                core = atoi(optarg);
                break;
            }

            case 'k':
            {
                size_in_kb = atoi(optarg);
                break;
            }

            case 'f':
            {
                output_file = fopen(optarg, "a");
                break;
            }
        }
    }

    if(size_in_kb == 0)
    {
        printf("invalid or missing arguments\n");
        return -1;
    }

    if(output_file == NULL)
    {
        output_file = fopen("results.csv", "a");
    }

    srandom(time(NULL));
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(core, &set);
	if (sched_setaffinity(getpid(), sizeof(set), &set) != 0)
    {
	    printf("failed to assign cpu core\n");
		return -1;
	}

    const uint64_t size_in_bytes = size_in_kb * 1024;
    uint64_t* array = (uint64_t*) malloc(size_in_bytes);

    unsigned int array_len = size_in_bytes / sizeof(uint64_t);

    double average_time = 0;
    for(unsigned int r = 0; r < NUMBER_OF_RUNS; r++)
    {
        // make sure the array is in the L1 cache and TLB
        for(int i = array_len - 1; i >= 0; i--)
        {
            array[i] = 0;
        }
        
        struct timespec start = {};
        struct timespec end   = {};

        clock_gettime(CLOCK_MONOTONIC, &start);

        // these loops are designed to skip 16 integers at a time in order avoid linear accesses and to skip over cache lines bought in by the prefetcher
        for(unsigned int i = 0; i < 16; i++)
        {
            for(unsigned int j = i; j < array_len; j += 16)
            {
                array[j] = j;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        double seconds = end.tv_sec - start.tv_sec;
        double nanoseconds = end.tv_nsec - start.tv_nsec;
        if(end.tv_nsec >= start.tv_nsec)
        {
            seconds -= 1;
            nanoseconds += 1000000000;
        }

        average_time += seconds + nanoseconds / 1000000000.0;
    }

    average_time /= (double) NUMBER_OF_RUNS;

    if(output_file == NULL)
    {
        printf("io failure\n");
        return -1;
    }
    fprintf(output_file, "%u,%f\n", size_in_kb, ((double) size_in_bytes / (1024.0f * 1024.0f * 1024.0f)) / average_time);
    fclose(output_file);

    free(array);
    return 0;
}


#define _GNU_SOURCE

#include <sched.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define KB (1024UL)
#define MB (1024UL * KB)
#define GB (1024UL * MB)

#define BYTES_TO_KB(bytes) (((double) (bytes)) / ((double) KB))
#define BYTES_TO_MB(bytes) (((double) (bytes)) / ((double) MB))
#define BYTES_TO_GB(bytes) (((double) (bytes)) / ((double) GB))

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

    const uint64_t size_in_bytes = size_in_kb * KB;
    uint64_t* array = (uint64_t*) malloc(size_in_bytes);

    const unsigned int NUM_RUNS = 10;
    unsigned int length = size_in_bytes / sizeof(uint64_t);

    double time = 0;
    struct timespec start_time = {}, end_time = {};

    for(unsigned int run = 0; run < NUM_RUNS; run++)
    {
        // "warm up" the array by writing to it backwards; write backwards to ensure the front of the array is in the L1 cache
        for(int i = length - 1; i >= 0; i--)
        {
            array[i] = 0;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        const unsigned int step_size = 16; // most prefetechers pull 2 cache lines at once, so to skip 2 cache lines, we must skip at least 16 64-bit integers (128 bytes)
        // write with this pattern: [1] [3] [5] [7] [9] [11] [13] [15] [17] [19] [21] [23] [25] [27] [29] [31] | [2] [4] [6] [8] [10] [12] [14] [16] [18] [20] [22] [24] [26] [28] [30] [32]
        for(unsigned int i = 0; i < step_size; i++)
        {
            for(unsigned int j = i; j < length; j += step_size)
            {
                array[j] = j;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end_time);

        double seconds = end_time.tv_sec - start_time.tv_sec;
        double nanoseconds = end_time.tv_nsec - start_time.tv_nsec;
        if(end_time.tv_nsec >= start_time.tv_nsec)
        {
            seconds -= 1;
            nanoseconds += 1000000000;
        }

        time += seconds + nanoseconds / 1000000000.0;
    }

    time /= (double) NUM_RUNS;

    if(output_file == NULL)
    {
        printf("IO failure\n");
        return -1;
    }
    fprintf(output_file, "%u,%f\n", size_in_kb, BYTES_TO_GB(size_in_bytes) / time);
    fclose(output_file);

    free(array);

    return 0;
}

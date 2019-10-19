
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

    int op = 0;
    while((op = getopt(argc, argv, "c:k:")) != -1)
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
        }
    }

    if(size_in_kb == 0)
    {
        printf("invalid or missing arguments\n");
        return -1;
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

    for(int i = array_len - 1; i >= 0; i--)
    {
        array[i] = 0;
    }

    for(unsigned int r = 0; r < NUMBER_OF_RUNS; r++)
    {
        // these loops are designed to skip 32 integers (2 cache lines) at a time to nullify prefetcher
        for(unsigned int i = 0; i < 16; i++)
        {
            for(unsigned int j = i; j < array_len; j += 16)
            {
                array[j] = j;
            }
        }
    }

    free(array);
    return 0;
}
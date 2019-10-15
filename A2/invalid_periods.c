
#include <sys/types.h>

#include <stdio.h>

#include "tsc.h"

u_int64_t find_threshold()
{
    const unsigned int NUM_RUNS = 100;

    u_int64_t curr = 0, prev = get_counter();

    u_int64_t sum = 0;
    u_int64_t diff = 0;
    for(unsigned int i = 0; i < NUM_RUNS;)
    {
        curr = get_counter();
        diff = curr - prev;

        if(diff > 500)
        {
            i++;
            sum += diff;
        }
        prev = curr;
    }

    return (u_int64_t) (((double) sum) / ((double) NUM_RUNS));
}

u_int64_t inactive_periods(int num, u_int64_t threshold, u_int64_t* samples)
{
    u_int64_t curr = 0, prev = get_counter();

    u_int64_t periods_inactive = 0;
    while(periods_inactive < num)
    {
        curr = get_counter();
        if((curr - prev) > threshold)
        {
            printf("wake up after %lu cycles\n", curr - prev);
            periods_inactive ++;
        }
        prev = curr;
    }
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("./inactive_periods num_periods\n");
        return -1;
    }
    else
    {
        int num_periods = 0;
        sscanf(argv[1], "%i", &num_periods);

        if(num_periods == 0)
        {
            printf("invalid argument\n");
        }
        else
        {
            start_counter();

            u_int64_t threshold = find_threshold();
            printf("threshold = %lu\n", threshold);
            inactive_periods(num_periods, threshold, NULL);
        }
    }

    return 0;
}

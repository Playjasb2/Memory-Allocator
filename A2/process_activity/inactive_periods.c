
#define _GNU_SOURCE

#include <sched.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tsc.h"

u_int64_t find_threshold()
{
    const unsigned int NUM_RUNS = 50;

    u_int64_t array[NUM_RUNS];
    u_int64_t curr = 0, prev = 0, diff = 0;

    start_counter();    
    for(unsigned int i = 0; i < NUM_RUNS;)
    {
        curr = get_counter();

        diff = curr - prev;
        if((diff > 1000) && (diff < 15000))
        {
            array[i++] = diff;
        }
        
        prev = curr;
    }

    u_int64_t sum = 0;
    for(unsigned int i = 0; i < NUM_RUNS; i++)
    {
        // printf("%lu\n", array[i]);
        sum += array[i];
    }

    u_int64_t threshold = (u_int64_t) ((float) sum / (float) NUM_RUNS);
    threshold = (float) threshold / 2.0f;

    printf("threshold = %lu\n", threshold);
    return threshold;
}

u_int64_t inactive_periods(int num, u_int64_t threshold, u_int64_t* samples)
{
    start_counter();
    u_int64_t start_time = get_counter();

    u_int64_t curr = 0, prev = 0, diff = 0;
    u_int64_t active_end = start_time;

    for(u_int64_t periods_inactive = 0; periods_inactive < num;)
    {
        curr = get_counter();

        diff = curr - prev;
        if(diff > threshold)
        {
            samples[periods_inactive * 2 + 0] = active_end;
            samples[periods_inactive * 2 + 1] = curr;

            periods_inactive++;
            active_end = curr;
        }
        else
        {
            active_end += diff;
        }

        prev = curr;
    }

    return start_time;
}

double find_clock_speed()
{
    const unsigned int NUM_RUNS = 5;

    struct timespec duration;
    duration.tv_sec = 0;
    duration.tv_nsec = 50000000; // 0.05 of a second

    double speed = 0.0f;

    for(unsigned int i = 0; i < NUM_RUNS; i++)
    {
        start_counter();
        nanosleep(&duration, NULL);
        speed += (double) get_counter() / 50000000;
    }

    speed = speed / ((float) NUM_RUNS); // cycles per second
    printf("speed (ghz) = %f\n", speed);

    speed *= 1000000; // 1 million cycles per millisecond
    printf("cycles per millisecond = %f\n", speed);

    return speed;
}

int main(int argc, char* argv[])
{
    srandom(time(NULL));
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET((random() ?: 1) % get_nprocs(), &set);
	if (sched_setaffinity(getpid(), sizeof(set), &set) != 0)
    {
	    printf("failed to assign cpu core\n");
		return -1;
	}

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
            u_int64_t threshold = find_threshold();
            double clock_speed = find_clock_speed();

            u_int64_t* samples = (u_int64_t*) malloc(sizeof(u_int64_t) * num_periods * 2);
            u_int64_t start = inactive_periods(num_periods, threshold, samples);
            
            FILE* f_out = fopen("results.csv", "w");
            if(f_out == NULL)
            {
                printf("io error\n");
                return -1;
            }

            for(unsigned int i = 0; i < num_periods; i++)
            {
                u_int64_t inactive_start = samples[i * 2 + 0];
                u_int64_t inactive_end   = samples[i * 2 + 1];

                u_int64_t active_length   = inactive_start - start;
                u_int64_t inactive_length = inactive_end - inactive_start;

                printf("Active %u: start at %lu, duration %lu cycles (%f ms)\n", i, start, active_length, ((double) active_length) / clock_speed);
                printf("Inactive %u: start at %lu, duration %lu cycles (%f ms)\n", i, inactive_start, inactive_length, ((double) inactive_length) / clock_speed);
                printf("\n");

                fprintf(f_out, "%f,%f\n", (double) start / clock_speed, (double) (start + active_length) / clock_speed);
                fprintf(f_out, "%f,%f\n", (double) inactive_start / clock_speed, (double) (inactive_start + inactive_length) / clock_speed);

                start = inactive_end;
            }
        }
    }

    return 0;
}

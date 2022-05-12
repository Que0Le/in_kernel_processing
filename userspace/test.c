#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>

#include "helper.h"

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

unsigned int max_prime = 10000;
unsigned int task_size = 30000;
unsigned int num_threads = 7;
unsigned int update_rate = 3;

static volatile int keepRunning = 1;
static volatile int threadHasTerminated = 0;

void intHandler(int dummy)
{
    keepRunning = 0;
}

void print_usage()
{
    printf("Usage: -m 1000 -s 10 -n 10 -r 2\n");
}

static unsigned long get_nsecs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000UL + ts.tv_nsec;
}

struct args2
{
    int id;
    int policy;
    int priority_value; // 1-99 for realtime policies RR and FIFO
    int task_size;
    unsigned long time_needed_last_task;
    unsigned long sum_time_to_current_process;
    double current_progress;
    int done;
};

void *task_process(void *input)
{
    unsigned long begin_process = get_nsecs();

    //int id = ((struct args2 *)input)->id;
    int t_size = ((struct args2 *)input)->task_size;
    for (int k = 1; k <= t_size; k++)
    {
        if (keepRunning)
        {
            //unsigned long start_prime = get_nsecs();
            // do_primes(max_prime);
            encrypt_bf();
            //unsigned long done_prime = get_nsecs();
            //unsigned long delta = done_prime - start_prime;
            ((struct args2 *)input)->sum_time_to_current_process = get_nsecs() - begin_process;
            ((struct args2 *)input)->current_progress = k;
            //((struct args2 *)input)->time_needed_last_task = delta;
        } else {
            break;
        }
    }
    ((struct args2 *)input)->done = 1;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int opt = 0;
    static struct option long_options[] = {
            {"max_prime", optional_argument, 0, 'm'},
            {"task_size", optional_argument, 0, 's'},
            {"num_theads", optional_argument, 0, 'n'},
            {"update_rate", optional_argument, 0, 'r'},
            {0, 0, 0, 0}};

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "m:s:n:r:",
                              long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'm':
                max_prime = atoi(optarg);
                break;
            case 's':
                task_size = atoi(optarg);
                break;
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 'r':
                update_rate = atoi(optarg);
                break;
            default:
                print_usage();
                exit(EXIT_FAILURE);
        }
    }

    int policy;
    struct args2 *param;
    pthread_t tid;
    pthread_attr_t attr;

    /* create  the param*/
    param = (struct args2 *)malloc(sizeof(struct args2));
    param->id = 0;
    param->task_size = task_size;
    param->time_needed_last_task = 0;
    param->current_progress = 0;
    param->done = 0;
    param->sum_time_to_current_process = 0;

    struct sched_param sch_param;
    if (pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0)
                fprintf(stderr, "Unable to set policy.\n");
    sch_param.sched_priority = 99;
    if (pthread_attr_setschedparam(&attr, &sch_param) != 0)
        fprintf(stderr, "Unable to set priority.\n");
    param->policy = SCHED_RR;
    param->priority_value = sch_param.sched_priority;

    pthread_create(&tid, &attr, task_process, (void *)param);
    signal(SIGINT, intHandler);

    pthread_join(tid, NULL);
    free(param);
    
    return 0;
}

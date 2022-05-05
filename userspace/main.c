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

void do_primes(unsigned int max_limit)
{
    unsigned long i, num, primes = 0;
    for (num = 1; num <= max_limit; ++num)
    {
        for (i = 2; (i <= num) && (num % i != 0); ++i)
            ;
        if (i == num)
            ++primes;
    }
    // printf("Calculated %d primes.\n", primes);
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

    int i, policy;
    struct args2 *params[num_threads];
    pthread_t tid[num_threads];
    pthread_attr_t attrs[num_threads];

    /* create  the param*/
//    int step = (num_threads-1)/2;
    for (i = 0; i < num_threads; i++)
    {
        struct sched_param sch_param;
        params[i] = (struct args2 *)malloc(sizeof(struct args2));
        params[i]->id = i;
        params[i]->task_size = task_size;
        params[i]->time_needed_last_task = 0;
        params[i]->current_progress = 0;
        params[i]->done = 0;
        params[i]->sum_time_to_current_process = 0;

        pthread_attr_init(&attrs[i]);

        if (pthread_attr_getschedpolicy(&attrs[i], &policy) != 0)
            fprintf(stderr, "Unable to get policy.\n");
        else
        {
            if(policy == SCHED_OTHER)
                printf("SCHED_OTHER\n");
            else if(policy == SCHED_RR)
                printf("SCHED_RR\n");
            else if(policy == SCHED_FIFO)
                printf("SCHED_FIFO\n");
        }
        if (i == 0) {
            // Leave it SCHED_OTHER
            params[i]->policy = SCHED_OTHER;
        }
        else if (i % 2 == 0)
        {
            if (pthread_attr_setschedpolicy(&attrs[i], SCHED_RR) != 0)
                fprintf(stderr, "Unable to set policy.\n");
            sch_param.sched_priority = (int) 99/(i-1);
            if (pthread_attr_setschedparam(&attrs[i], &sch_param) != 0)
                fprintf(stderr, "Unable to set priority.\n");
            params[i]->policy = SCHED_RR;
            params[i]->priority_value = sch_param.sched_priority;
        }
        else
        {
            if (pthread_attr_setschedpolicy(&attrs[i], SCHED_FIFO) != 0)
                fprintf(stderr, "Unable to set policy.\n");
            sch_param.sched_priority = (int) 99/(i);
            if (pthread_attr_setschedparam(&attrs[i], &sch_param) != 0)
                fprintf(stderr, "Unable to set priority.\n");
            params[i]->policy = SCHED_FIFO;
            params[i]->priority_value = sch_param.sched_priority;
        }
    }

    /* create the threads */
//    for (i = 0; i < num_threads; i++)
//    {
//        pthread_create(&tid[i], &attrs[i], task_process, (void *)params[i]);
//    }
    uint64_t max = 0;
    uint64_t min = RAND_MAX;
    uint64_t sum = 0;
    /* create the threads */
    for (i = 0; i < num_threads; i++)
    {
        uint64_t start_create_thread = get_nsecs();
        pthread_create(&tid[i], &attrs[i], task_process, (void *)params[i]);
        uint64_t end_create_thread = get_nsecs();
        uint64_t delta = end_create_thread-start_create_thread;
        sum+=delta;
        if (delta<=min) {
            min=delta;
        }
        if (delta>=max) {
            max=delta;
        }
//        printf("end_create_thread-start_create_thread=%lu\n", delta);
    }
    printf("min=%lu, max=%lu, avg=%lu\n", min, max, sum/num_threads);

    /* Kill program in a soft manner:
     * printing data collected until now before quit. See task_process()
     */
    signal(SIGINT, intHandler);

    /* Number of thread that has done computing and exited */
    int count = 0;
//    unsigned long sum_FIFO_current_iter_win = 0;
//    unsigned long sum_RR_current_iter_win = 0;
    unsigned long last_iter = get_nsecs();
    unsigned long now;

    while (keepRunning && (count <= num_threads)) {
        // printf("%d\n", count);
        count = 0;
        /* Gathering data */
        unsigned long sum_FIFO_current_iter = 0;
        unsigned long sum_RR_current_iter = 0;
        unsigned long sum_FIFO = 0;
        unsigned long sum_RR = 0;
        for (i = 0; i < num_threads; i++)
        {
            if (params[i]->policy % 2 == 0)
            {
                sum_RR_current_iter += params[i]->time_needed_last_task;
                sum_RR += params[i]->sum_time_to_current_process;
            }
            else
            {
                sum_FIFO_current_iter += params[i]->time_needed_last_task;
                sum_FIFO += params[i]->sum_time_to_current_process;
            }

            if (params[i]->done == 1) {
                count++;
            }
        }

        /* Who won last task. This is NOT a precise metres! */
//        if (sum_RR_current_iter>sum_FIFO_current_iter) {
//            sum_FIFO_current_iter_win++;
//        } else {
//            sum_RR_current_iter_win++;
//        }

        /* Print out result after update_rate (sec) */
        now = get_nsecs();
        if ((count == num_threads) || (now - last_iter) > (update_rate * 1000000000))
        {
            printf("-------------------------------------------\n");
            for (i = 0; i < num_threads; i++)
            {
                //printf("time_needed_last_task for thread id= %d: %lu", i, params[i]->time_needed_last_task);
                printf("sum_time_to_current_process for thread id= %d: %lu", i, params[i]->sum_time_to_current_process);
                printf(" current_progress: %.4f%% ", 100 * params[i]->current_progress / task_size);
                if (params[i]->policy == SCHED_OTHER)
                {
                    printf(" SCHED_OTHER");
                }
                else if (params[i]->policy == SCHED_RR)
                {
                    printf(" SCHED_RR  ");
                }
                else if (params[i]->policy == SCHED_FIFO)
                {
                    printf(" SCHED_FIFO");
                }
                printf(" priority_value %d", params[i]->priority_value);
                printf("\n");
            }
//            printf("sum_FIFO_last_task won (estimate) %lu checks, total time cal prime:  = %lu nsec\n", sum_FIFO_current_iter_win, sum_FIFO);
//            printf("sum_RR_last_task   won (estimate) %lu checks, total time cal prime:  = %lu nsec\n", sum_RR_current_iter_win, sum_RR);

            /* Ensure to print the 100% */
            if (count == num_threads) {
                break;
            }
            last_iter = now;
        }
    }

    /* now join on each thread */
    for (i = 0; i < num_threads; i++)
        pthread_join(tid[i], NULL);

    /* Free mem */
    for (i = 0; i < num_threads; i++)
    {
        free(params[i]);
    }

    return 0;
}

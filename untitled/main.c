#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include "helper.h"

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

unsigned long task_repeat = 10000;
unsigned long task_size = 30000;
unsigned long sleep_time_usec = 30000;
unsigned int num_threads = 7;
unsigned int update_rate = 3;

struct args2 {
    int id;
    int policy;
    int priority_value; // 1-99 for realtime policies RR and FIFO
    unsigned long task_size;
    unsigned long task_repeat;
    unsigned long time_needed_last_task;
    unsigned long sum_time_to_current_process;
    double current_progress;
    unsigned long SUM_TIME_TASK_PROCESSING;
    unsigned long COUNT_TASK_PROCESSING;
    unsigned long SUM_TIME_SLEEP;
    unsigned long COUNT_SLEEP;
    unsigned long NANO_TIME_SLEEP;
    unsigned long failure_sleep;
    int done;
};

void *task_process(void *input) {
    struct timespec ts_requested;
    struct timespec ts_remaining;
    unsigned long start_encrypt = 0;
    unsigned long done_encrypt = 0;
    unsigned long waked = 0;
    int return_code;

    unsigned long t_size = ((struct args2 *) input)->task_size;
    unsigned long t_repeat = ((struct args2 *) input)->task_repeat;

    ts_requested.tv_sec  = ((struct args2 *) input)->NANO_TIME_SLEEP / 1000000000UL;
    ts_requested.tv_nsec = ((struct args2 *) input)->NANO_TIME_SLEEP % 1000000000UL;

    for (int k = 1; k <= t_repeat; k++) {
        if (keepRunning != 1) {
            printf("Break by keepRunning = %d!", keepRunning);
            break;
        }
        ((struct args2 *) input)->current_progress += 1;
        start_encrypt = get_nsecs();
        for (int j = 0; j < t_size; j++) {
            encrypt_bf();
        }
        done_encrypt = get_nsecs();
        return_code = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts_requested, &ts_remaining);
        waked = get_nsecs();;
        if (return_code != 0) {
            ((struct args2 *) input)->failure_sleep++;
            printf("`clock_nanosleep()` failed! return_code = %i: ", return_code);
            // See all error codes here: https://man7.org/linux/man-pages/man2/clock_nanosleep.2.html
            switch (return_code) {
                case EFAULT:
                    printf("EFAULT: `request` or `remain` specified an invalid address.\n");
                    break;
                case EINTR:
                    printf("EINTR: The sleep was interrupted by a signal handler.\n");
                    break;
                case EINVAL:
                    printf("EINVAL: The value in the `tv_nsec` field was not in the range 0 to "
                           "999999999 or `tv_sec` was negative.\n");
                    break;
                case ENOTSUP:
                    printf("ENOTSUP: The kernel does not support sleeping against this `clockid`.\n");
                    break;
                default:
                    printf("Default\n");
                    break;
            }
        }
        ((struct args2 *) input)->SUM_TIME_TASK_PROCESSING += done_encrypt - start_encrypt;
        ((struct args2 *) input)->COUNT_TASK_PROCESSING += 1;
        ((struct args2 *) input)->SUM_TIME_SLEEP += waked - done_encrypt;
        ((struct args2 *) input)->COUNT_SLEEP += 1;
    }
    ((struct args2 *) input)->done = 1;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int opt = 0;
    static struct option long_options[] = {
            {"task_repeat", optional_argument, 0, 'r'},
            {"task_size",   optional_argument, 0, 's'},
            {"sleep_time_usec",  optional_argument, 0, 't'},
            {"update_rate", optional_argument, 0, 'u'},
            {0, 0,                             0, 0}};

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "r:s:t:u:",
                              long_options, &long_index)) != -1) {
        switch (opt) {
            case 'r':
                task_repeat = atoi(optarg);
                break;
            case 's':
                task_size = atoi(optarg);
                break;
            case 't':
                sleep_time_usec = atoi(optarg);
                break;
            case 'u':
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
    param = (struct args2 *) malloc(sizeof(struct args2));
    param->id = 0;
    param->task_size = task_size;
    param->task_repeat = task_repeat;
    param->time_needed_last_task = 0;
    param->current_progress = 0;
    param->done = 0;
    param->sum_time_to_current_process = 0;
    param->NANO_TIME_SLEEP = sleep_time_usec * 1000;

    struct sched_param sch_param;
    if (pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0)
        fprintf(stderr, "Unable to set policy.\n");
    sch_param.sched_priority = 99;
    if (pthread_attr_setschedparam(&attr, &sch_param) != 0)
        fprintf(stderr, "Unable to set priority.\n");
    param->policy = SCHED_RR;
    param->priority_value = sch_param.sched_priority;

    pthread_create(&tid, NULL, task_process, param);
//    signal(SIGINT, intHandler);

    unsigned long next_ite_to_print = 0;
    while (param->done == 0 && next_ite_to_print <= 100) {
        next_ite_to_print = print_progress(
                task_repeat,
                param->current_progress, next_ite_to_print
        );
        if (next_ite_to_print > 100)
            break;
        usleep(100);
    }


    pthread_join(tid, NULL);
    printf("AVG Processing time  : %f\n",
           (double) param->SUM_TIME_TASK_PROCESSING / param->COUNT_TASK_PROCESSING
    );
    printf("AVG Sleep time       : %f = x%f of input\n",
           (double) param->SUM_TIME_SLEEP / param->COUNT_SLEEP,
           (double) (param->SUM_TIME_SLEEP / param->COUNT_SLEEP) / param->NANO_TIME_SLEEP
    );
    printf("Sleep time requested : %lul\n", param->NANO_TIME_SLEEP);
    free(param);

    return 0;
}

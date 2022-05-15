//
// Created by que on 5/12/22.
//

#ifndef UNTITLED_HELPER_H
#define UNTITLED_HELPER_H

#endif //UNTITLED_HELPER_H

static volatile int keepRunning = 1;
static volatile int threadHasTerminated = 0;

void intHandler(int dummy);
void print_usage();
unsigned long get_nsecs(void);
int test_helper();

int encrypt_bf(void);
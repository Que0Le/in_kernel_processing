//
// Created by que on 5/12/22.
//

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "blowfish.h"
#include "helper.h"

void intHandler(int dummy)
{
    keepRunning = 0;
}

void print_usage()
{
    printf("Usage: -r 100 -s 100 -t 100000 -u 1\n");
}

unsigned long get_nsecs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000UL + ts.tv_nsec;
}

int encrypt_bf(void) {
    BLOWFISH_CTX ctx;
    int i;
    char plaintext[] =
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";

    char encrypted[strlen(plaintext) + 1];
    memset(encrypted, '\0', (int)strlen(plaintext) + 1);
    char decrypted[strlen(plaintext) + 1];
    memset(decrypted, '\0', (int)strlen(plaintext) + 1);

    char salt[] = "TESTKEY";

    Blowfish_Init(&ctx, (unsigned char *)salt, strlen(salt));

    unsigned long L, R = 2;
    for (i = 0; i < strlen(plaintext); i += 8) {
        // Copy L and R
        memcpy(&L, &plaintext[i], 4);
        memcpy(&R, &plaintext[i + 4], 4);
        // Encrypt the parts
        Blowfish_Encrypt(&ctx, &L, &R);
        // Store the output
        memcpy(&encrypted[i], &L, 4);
        memcpy(&encrypted[i + 4], &R, 4);
        // Decrypt
        Blowfish_Decrypt(&ctx, &L, &R);
        // Store result
        memcpy(&decrypted[i], &L, 4);
        memcpy(&decrypted[i + 4], &R, 4);
    }

    return 0;
}
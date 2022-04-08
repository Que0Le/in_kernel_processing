#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
// #include <sys/types.h>   // ssize_t
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "blowfish.h"

static void test_func(int arg)
{
    pr_info("test function");
}

#define CHARS_LENGTH 100
static char mychars[CHARS_LENGTH] = "\0";
static ssize_t mychars_show(struct kobject *kobj, struct kobj_attribute *attr,
                            char *buf)
{
    // return sprintf(buf, "%d\n", mychars);
    return snprintf(buf, CHARS_LENGTH, "%s", mychars);
}

static ssize_t mychars_store(struct kobject *kobj, struct kobj_attribute *attr,
                             char *buf, size_t count)
{
    // sscanf(buf, "%du", &mychars);
    if (count < CHARS_LENGTH) {
        // snprintf(buf, CHARS_LENGTH, "%s", mychars);
        strncpy(mychars, buf, count);
    } else {
        strncpy(mychars, buf, CHARS_LENGTH - 1);
        mychars[CHARS_LENGTH] = "\0";
    }
    return count;
}

/////////////////////////////////////////////////////////////////////////////
static struct kobject *mymodule;

/* the variable you want to be able to change */
static int myvariable = 0;

static struct kobj_attribute mychars_attribute =
    __ATTR(mychars, 0660, mychars_show, (void *)mychars_store);

///////////////////////
static ssize_t myvariable_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", myvariable);
}

static ssize_t myvariable_store(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf,
                                size_t count)
{
    sscanf(buf, "%du", &myvariable);
    return count;
}

static struct kobj_attribute myvariable_attribute =
    __ATTR(myvariable, 0660, myvariable_show, (void *)myvariable_store);
/////////////////////////////////////////////////////////////////////////////////

static int encrypt_bf(void)
{
    char plaintext[] =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";

    char encrypted[strlen(plaintext)];
    memset(encrypted, '\0', (int)strlen(plaintext));
    char decrypted[strlen(plaintext)];
    memset(decrypted, '\0', (int)strlen(plaintext));

    BLOWFISH_CTX ctx;
    char salt[] = "TESTKEY";

    Blowfish_Init(&ctx, (unsigned char *)salt, strlen(salt));

    int i;
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

    pr_info("plaintext:\n%.*s\n", (int)strlen(plaintext), plaintext);
    pr_info("---------------------------------------------------\n");
    pr_info("encrypted:\n%.*s\n", (int)strlen(plaintext), encrypted);
    pr_info("---------------------------------------------------\n");
    pr_info("decrypted:\n%.*s\n", (int)strlen(plaintext), decrypted);
    pr_info("\n\n\n");

    return 0;
}
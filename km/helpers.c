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
#include <linux/preempt.h>
#include <linux/delay.h>

#include "blowfish.h"


static int keep_running = 1;
static int work_running = 0;


struct work_blowfish {
    struct work_struct real_work;
    int nbr_iteration;
    int workloads;  // nbr of workload repeated in each iteration
    int sleep_ms;
    int sleep_type;
};

static struct work_blowfish *work_bf;



/////////////////////////////////////////////////////////////////////////////
static struct kobject *mymodule;

/* the variable you want to be able to change */
static int myvariable = 0;

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

    return 0;
}


static void print_is_in_task(void) {
    if (!in_task())
        pr_info("[in_kernel_processing] Code running in atomic or interrupt context.");
    else {
        pr_info("[in_kernel_processing] Code in process (or task) context.");
    }
}

static void selective_sleep(int type, unsigned long value) {
    switch(type) {
        case 1:
            msleep(value);
            break;
        case 2:
            msleep_interruptible(value);
            break;
        case 3:
            usleep_range(value*1000, (unsigned long) value*1500);
            break;
        default:
            msleep(value);
    } 
}


// struct work_blowfish *work_bf;
static int work_blowfish_keep_running = 1;

static void work_blowfish_handler(struct work_struct *work_arg){
    int i;
	struct work_blowfish *c_ptr = container_of(work_arg, struct work_blowfish, real_work);
    work_running = 1;
    printk(KERN_INFO "[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
    // printk(KERN_INFO "[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
    print_is_in_task();
    unsigned long start_processing, done_processing, waked;
    for (i=0; i<c_ptr->nbr_iteration; i++) {
        int j;
        if (keep_running != 1) {
            pr_alert("Break by keep_running = %d!", keep_running);
            break;
        }
        pr_alert("... processing");
        start_processing = ktime_get_ns();
        for (j=0; j<c_ptr->workloads; j++)
            encrypt_bf();
        done_processing = ktime_get_ns();
        set_current_state(TASK_INTERRUPTIBLE);
        // schedule_timeout(2 * HZ); //Wait 2 seconds
        selective_sleep(c_ptr->sleep_type, c_ptr->sleep_ms);
        waked = ktime_get_ns();
        pr_info("start->done: %lu | done->waked: %lu", 
                    (unsigned long)(done_processing-start_processing)/1000,
                    (unsigned long)(waked-done_processing)/1000);
    }
    // grep 'CONFIG_HZ=' /boot/config-$(uname -r)  # CONFIG_HZ=250
	
	printk(KERN_INFO "[Deferred work]=> DONE. BTW the run iteration is: %d/%d\n", i, c_ptr->nbr_iteration);
    work_running = 0;
	return;
}

// set_current_state() changes the state of the currently executing process 
// from TASK_RUNNING to TASK_INTERRUPTIBLE.
// TASK_RUNNING -- A ready-to-run process has the state TASK_RUNNING.
// TASK_INTERRUPTIBLE -- A state of the process, with which is schedule() is called, the process is moved off the run-queue.


#define CHARS_LENGTH 100
static char mychars[CHARS_LENGTH] = "\0";
static ssize_t mychars_show(struct kobject *kobj, struct kobj_attribute *attr,
                            char *buf)
{
    // return sprintf(buf, "%d\n", mychars);
    return snprintf(buf, CHARS_LENGTH, "%s", mychars);
}

static int mychars_store(struct kobject *kobj, struct kobj_attribute *attr,
                             char *buf, size_t count)
{
    // sscanf(buf, "%du", &mychars);
    if (count < CHARS_LENGTH) {
        // snprintf(buf, CHARS_LENGTH, "%s", mychars);
        strncpy(mychars, buf, count);
        mychars[count] = "\0";
    } else {
        strncpy(mychars, buf, CHARS_LENGTH - 1);
        mychars[CHARS_LENGTH] = "\0";
    }
    //
    if (keep_running == 1)
        keep_running = 0;
    int try = 5;
    while(try > 0) {
        try -= 1;
        if (work_running == 1) {
            pr_info("Old work still running. Waiting ...");
            msleep(500);
        } else {
            break;
        }
        if (try == 0) {
            pr_alert("Can't add new work: running work didn't stop after 5 tries!");
            return -1;
        }
    }
    // Add work
    INIT_WORK(&work_bf->real_work, work_blowfish_handler);
    work_bf->nbr_iteration = 10;
    work_bf->workloads = 20;
    work_bf->sleep_type = 1;
    work_bf->sleep_ms = 100;
    keep_running = 1;
    schedule_work(&work_bf->real_work);

    return count;
}

static struct kobj_attribute mychars_attribute =
    __ATTR(mychars, 0660, mychars_show, (void *)mychars_store);
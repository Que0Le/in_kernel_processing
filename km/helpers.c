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
#include "helpers.h"

static int keep_running = 1;
static int work_running = 0;

static unsigned long SUM_TIME_SLEEP = 0;
static unsigned long COUNT_SLEEP = 0;
static unsigned long SUM_TIME_TASK_PROCESSING = 0;
static unsigned long COUNT_TASK_PROCESSING = 0;

struct work_blowfish {
    struct work_struct real_work;
    int task_repeat;
    int task_size;  // nbr of workload repeated in each iteration
    int sleep_usec;
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


static void print_is_in_task(void) {
    if (!in_task())
        pr_info("[in_kernel_processing] Code running in atomic or interrupt context.");
    else {
        pr_info("[in_kernel_processing] Code in process (or task) context.");
    }
}

static void selective_sleep_in_usec(int type, unsigned long value) {
    switch(type) {
        case 1:
            msleep((unsigned int) value/1000);
            break;
        case 2:
            msleep_interruptible((unsigned int) value/1000);
            break;
        case 3:
            usleep_range(value, (unsigned long) value*12/10);
            break;
        default:
            msleep((unsigned int) value/1000);
    } 
}


static void test_work_handler(struct work_struct *work_arg) {
	struct work_blowfish *c_ptr = container_of(work_arg, struct work_blowfish, real_work);
    work_running = 1;
    printk(KERN_INFO "Testing done: PID: %d; NAME: %s | work_blowfish: %d %d %d %d\n", 
                current->pid, current->comm,
                work_bf->sleep_type, work_bf->sleep_usec, 
                work_bf->task_repeat, work_bf->task_size
    );
    work_running = 0;
    return;
}

static unsigned long print_progress(
        unsigned long total_iteration,
        unsigned long current_iteration,
        unsigned long next_ite_to_print)
{
    if (current_iteration == 0 && next_ite_to_print == 0) {
        pr_alert("processing 0/100 ...\n");
        return 10;
    }
    if (current_iteration == (total_iteration - 1)) {
        pr_alert("processed 100/100!\n");
        return 100;
    }
    if ((unsigned long) (total_iteration * next_ite_to_print / 100) == (current_iteration + 10)) {
        pr_alert("processing %lu/100 ...\n", next_ite_to_print);
        return next_ite_to_print + 10;
    }
    return next_ite_to_print;
}


static void work_blowfish_handler(struct work_struct *work_arg) {
    SUM_TIME_SLEEP = 0;
    COUNT_SLEEP = 0;
    SUM_TIME_TASK_PROCESSING = 0;
    COUNT_TASK_PROCESSING = 0;
    unsigned long next_ite_to_print = 0;
    int i;
	struct work_blowfish *c_ptr = container_of(work_arg, struct work_blowfish, real_work);
    work_running = 1;
    printk(KERN_INFO "[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
    // printk(KERN_INFO "[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
    // print_is_in_task();
    unsigned long start_processing, done_processing, waked;
    pr_alert("... processing");
    for (i=0; i<c_ptr->task_repeat; i++) {
        int j;
        if (keep_running != 1) {
            pr_alert("Break by keep_running = %d!", keep_running);
            break;
        }
        start_processing = ktime_get_ns();
        for (j=0; j<c_ptr->task_size; j++)
            encrypt_bf();
        done_processing = ktime_get_ns();
        set_current_state(TASK_INTERRUPTIBLE);
        // schedule_timeout(2 * HZ); //Wait 2 seconds
        selective_sleep_in_usec(c_ptr->sleep_type, c_ptr->sleep_usec);
        waked = ktime_get_ns();
        // pr_info("start->done: %lu | done->waked: %lu", 
        //             (unsigned long)(done_processing-start_processing)/1000,
        //             (unsigned long)(waked-done_processing)/1000);
        COUNT_TASK_PROCESSING += 1;
        SUM_TIME_TASK_PROCESSING += done_processing-start_processing;
        COUNT_SLEEP += 1;
        SUM_TIME_SLEEP += waked-done_processing;
        next_ite_to_print = print_progress(c_ptr->task_repeat, i, next_ite_to_print);
              
    }
    // grep 'CONFIG_HZ=' /boot/config-$(uname -r)  # CONFIG_HZ=250

	pr_info("AVG Processing time  : %lu",  (unsigned long) SUM_TIME_TASK_PROCESSING/COUNT_TASK_PROCESSING);
	pr_info("AVG Sleep time       : %lu = x%lu.%lu of input", 
            (unsigned long) SUM_TIME_SLEEP / COUNT_SLEEP,
            (unsigned long) (SUM_TIME_SLEEP / COUNT_SLEEP)/(c_ptr->sleep_usec*1000),
            (unsigned long) (SUM_TIME_SLEEP / COUNT_SLEEP)%(c_ptr->sleep_usec*1000));
	printk(KERN_INFO "[Deferred work]=> DONE. BTW the run iteration is: %d/%d\n", i, c_ptr->task_repeat);
    work_running = 0;
	return;
}

// set_current_state() changes the state of the currently executing process 
// from TASK_RUNNING to TASK_INTERRUPTIBLE.
// TASK_RUNNING -- A ready-to-run process has the state TASK_RUNNING.
// TASK_INTERRUPTIBLE -- A state of the process, with which is schedule() is called, the process is moved off the run-queue.

static void input_to_work(char *text, size_t count, struct work_blowfish *work_bf) {
    // int iteration, sleep_style, workload;
    char buffs[4][10];
    memset(buffs, '\0', 4*10);

    int current_read = 0;
    int current_begin = 0;
    int i;
    for (i=0; i<count; i++) {
        if (text[i] == ',' || i == count-1) {
            memcpy(&buffs[current_read], &text[current_begin], i - current_begin);
            // printf("current_read=%d: %s\n", current_read, buffs[current_read]);
            current_begin = i+1;
            current_read += 1;
        }
    }
    sscanf(buffs[0], "%d", &work_bf->sleep_type);
    sscanf(buffs[1], "%d", &work_bf->sleep_usec);
    sscanf(buffs[2], "%d", &work_bf->task_repeat);
    sscanf(buffs[3], "%d", &work_bf->task_size);
}

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
    input_to_work(buf, count, work_bf);
    pr_alert("%d %d %d %d\n", work_bf->sleep_type, work_bf->sleep_usec, work_bf->task_repeat, work_bf->task_size);
    if (work_running == 1 && keep_running == 1)
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

    // // Add work
    INIT_WORK(&work_bf->real_work, work_blowfish_handler);
    // work_bf->nbr_iteration = 10;
    // work_bf->workloads = 20;
    // work_bf->sleep_type = 1;
    // work_bf->sleep_ms = 100;
    keep_running = 1;
    schedule_work(&work_bf->real_work);

    return count;
}

static struct kobj_attribute mychars_attribute =
    __ATTR(mychars, 0660, mychars_show, (void *)mychars_store);
/*
 * In kernel processing example
 */
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

#include "helpers.c"

static struct workqueue_struct *queue = NULL;
static struct work_struct work;

struct work_cont {
	struct work_struct real_work;
	int    arg;
} work_cont;

static void thread_function(struct work_struct *work);

struct work_cont *test_wq;

static void thread_function(struct work_struct *work_arg){
	struct work_cont *c_ptr = container_of(work_arg, struct work_cont, real_work);

	printk(KERN_INFO "[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
	printk(KERN_INFO "[Deferred work]=> I am going to sleep 2 seconds\n");
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(2 * HZ); //Wait 2 seconds
	
	printk(KERN_INFO "[Deferred work]=> DONE. BTW the data is: %d\n", c_ptr->arg);

	return;
}

static int __init mymodule_init(void)
{
    int error = 0;

    pr_info("In kernel processing: initialised\n");

    mymodule = kobject_create_and_add("mymodule", kernel_kobj);
    if (!mymodule)
        return -ENOMEM;

    error = sysfs_create_file(mymodule, &myvariable_attribute.attr);
    if (error) {
        pr_info("failed to create the myvariable file "
                "in /sys/kernel/mymodule\n");
    }
    error = sysfs_create_file(mymodule, &mychars_attribute.attr);
    if (error) {
        pr_info("failed to create the mychars file "
                "in /sys/kernel/mymodule\n");
    }

    work_bf = kmalloc(sizeof(*work_bf), GFP_KERNEL);

    // Create a testing work
	INIT_WORK(&work_bf->real_work, test_work_handler);
	work_bf->sleep_usec = 1;
	work_bf->sleep_type = 1;
	work_bf->task_repeat = 1;
	work_bf->task_size = 1;
	schedule_work(&work_bf->real_work);

    return error;
}

static void __exit mymodule_exit(void)
{
    flush_work(&work_bf->real_work);
	// kfree(test_wq);
	kfree(work_bf);

    pr_info("mymodule: Exit success\n");
    kobject_put(mymodule);
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");

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
#include <linux/workqueue.h>

static struct workqueue_struct *queue = NULL;
static struct work_struct work;

static void work_handler(struct work_struct *data)
{
    pr_info("work handler function.\n");
}

static int sched_init(void)
{
    queue = alloc_workqueue("HELLOWORLD", WQ_UNBOUND, 1);
    INIT_WORK(&work, work_handler);
    schedule_work(&work);
    return 0;
}

static int sched_exit(void)
{
    destroy_workqueue(queue);
    return 0;
}


static struct kobject *mymodule;

/* the variable you want to be able to change */
static int myvariable = 0;
#define CHARS_LENGTH 100
static char mychars[CHARS_LENGTH] = "\0";
static ssize_t mychars_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf)
{
    // return sprintf(buf, "%d\n", mychars);
    return snprintf(buf, CHARS_LENGTH, "%s", mychars);
}

static ssize_t mychars_store(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf,
                                size_t count)
{
    // sscanf(buf, "%du", &mychars);
    if (count < CHARS_LENGTH) {
        // snprintf(buf, CHARS_LENGTH, "%s", mychars);
        strncpy(mychars, buf, count);
    } else {
        strncpy(mychars, buf, CHARS_LENGTH-1);
        mychars[CHARS_LENGTH] = "\0";
    }
    return count;
}

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

static int __init mymodule_init(void)
{
    int error = 0;

    pr_info("mymodule: initialised\n");

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

    return error;
}

static void __exit mymodule_exit(void)
{
    pr_info("mymodule: Exit success\n");
    kobject_put(mymodule);
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");

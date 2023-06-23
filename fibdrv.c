#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/hash.h>
#include <linux/hashtable.h>
#include "BigNumber.h"

#include <linux/delay.h>    //used for ssleep()
#include <linux/kernel.h>   //used for do_exit()
#include <linux/kthread.h>  //used for kthread_create
#include <linux/threads.h>  //used for allow_signal

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

/*@brief
 * kthread parameters
 */

static struct task_struct *worker_task, *default_task;
static int get_current_cpu, set_current_cpu;
#define WORKER_THREAD_DELAY 4
#define DEFAULT_THREAD_DELAY 6
#define DEFAULT_HASHTABLE_LENGTH 7

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 1000000

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
// static DEFINE_MUTEX(fib_mutex);

/**
 The below section is for hashtable
 **/
struct bn_hashtable {
    unsigned int id;
    struct _bn *bn_object;
    struct hlist_node node;
};
/*
 * define a hash table with 2^7(=128) buckets
 * => struct hlist_head htable[128] = { [0 ... 127] = HLIST_HEAD_INIT };
 */
DEFINE_HASHTABLE(htable, DEFAULT_HASHTABLE_LENGTH);


static long long fib_sequence(long long k)
{
    /* FIXME: C99 variable-length array (VLA) is not allowed in Linux kernel. */
    long long f[k + 2];

    f[0] = 0;
    f[1] = 1;

    for (int i = 2; i <= k; i++) {
        f[i] = f[i - 1] + f[i - 2];
    }

    return f[k];
}
static long long fib_doubling(long long n)
{
    if (n <= 2)
        return n ? 1 : 0;
    long long k = n >> 1;
    long long a = fib_doubling(k);
    long long b = fib_doubling(k + 1);
    if (n % 2)
        return a * a + b * b;
    return a * ((b << 1) - a);
}
static long long fib_interative(long long n)
{
    uint8_t bits = 0;
    for (uint64_t i = n; i; ++bits, i >>= 1)
        ;

    long long a = 0;  // F(0) = 0
    long long b = 1;  // F(1) = 1
    for (uint64_t mask = 1 << (bits - 1); mask; mask >>= 1) {
        long long c = a * (2 * b - a);  // F(2k) = F(k) * [ 2 * F(k+1) – F(k) ]
        long long d = a * a + b * b;    // F(2k+1) = F(k)^2 + F(k+1)^2

        if (mask & n) {  // n_j is odd: k = (n_j-1)/2 => n_j = 2k + 1
            a = d;       //   F(n_j) = F(2k + 1)
            b = c + d;   //   F(n_j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
        } else {         // n_j is even: k = n_j/2 => n_j = 2k
            a = c;       //   F(n_j) = F(2k)
            b = d;       //   F(n_j + 1) = F(2k + 1)
        }
    }

    return a;
}
void bn_fib_recursive(bn *dest, unsigned int n)
{
    bn_resize(dest, 1);
    if (n < 2) {  // Fib(0) = 0, Fib(1) = 1
        dest->number[0] = n;
        return;
    }

    bn *a = bn_alloc(1);
    bn *b = bn_alloc(1);
    // bn_fib_recursive(a, n - 1);
    // bn_fib_recursive(b, n - 2);
    // bn_add(a, b, dest);
    bn_free(a);
    bn_free(b);
}
/* calc n-th Fibonacci number and save into dest */
void bn_fib(bn *dest, unsigned int n)
{
    bn_resize(dest, 1);
    if (n < 2) {  // Fib(0) = 0, Fib(1) = 1
        dest->number[0] = n;
        return;
    }

    bn *a = bn_alloc(1);
    bn *b = bn_alloc(1);
    dest->number[0] = 1;

    // unsigned int key = n;
    // struct bn_hashtable *hash_t_;
    // hash_for_each_possible(htable, hash_t_, node, key) {
    //     if(hash_t_->id == key) {
    //         dest->number = hash_t_->bn_object->number;
    //         dest->size = hash_t_->bn_object->size;
    //         break;
    //     }
    // }
    for (unsigned int i = 1; i < n; i++) {
        bn_swap(b, dest);    // b = dest
        bn_add(a, b, dest);  // dest = a + dest
        bn_swap(a, b);       // a = dest
        // struct bn_hashtable *hash_t = kmalloc(sizeof(struct bn_hashtable),
        // GFP_KERNEL); hash_t->id = i+1; bn *tmp = bn_alloc(1); bn_cpy(tmp,
        // dest); hash_t->bn_object = tmp; hash_add(htable, &hash_t->node,
        // hash_t->id);
    }
    bn_free(a);
    bn_free(b);
}
void bn_fib_doubling(bn *dest, unsigned int n)
{
    bn_resize(dest, 1);
    if (n < 2) {  // Fib(0) = 0, Fib(1) = 1
        dest->number[0] = n;
        return;
    }

    bn *f1 = dest;        /* F(k) */
    bn *f2 = bn_alloc(1); /* F(k+1) */
    f1->number[0] = 0;
    f2->number[0] = 1;
    bn *k1 = bn_alloc(1);
    bn *k2 = bn_alloc(1);
    // Follow the rule as below
    // f(0)  -----> f(1)  -----> f(2)  -----> f(5)  -----> f(10)  ......
    //        2n+1          2n          2n+1          2n
    unsigned int count = 0;
    struct bn_hashtable *hash_t =
        kmalloc(sizeof(struct bn_hashtable), GFP_KERNEL);
    hash_t->id = count;
    bn *tmp = bn_alloc(1);
    bn_cpy(tmp, dest);
    hash_t->bn_object = tmp;
    hash_add(htable, &hash_t->node, hash_t->id);
    // Take for example f(100) = 1100100 , initial i is 1100100, following i as
    // 1000000, 0100000, 0000000, 0000000, 0000100, 0000000, 0000000
    for (unsigned int i = 1U << (31 - __builtin_clz(n)); i; i >>= 1) {
        count = (n & i) ? (count << 1) + 1 : count << 1;
        /* F(2k) = F(k) * [ 2 * F(k+1) – F(k) ] */
        /* F(2k+1) = F(k)^2 + F(k+1)^2 */
        bn_lshift(f2, 1, k1);  // k1 = 2 * F(k+1)
        bn_sub(k1, f1, k1);    // k1 = 2 * F(k+1) – F(k)
        bn_mult(k1, f1, k2);   // k2 = k1 * f1 = F(2k)
        bn_mult(f1, f1, k1);   // k1 = F(k)^2
        bn_swap(f1, k2);       // f1 <-> k2, f1 = F(2k) now
        bn_mult(f2, f2, k2);   // k2 = F(k+1)^2
        bn_add(k1, k2, f2);    // f2 = f1^2 + f2^2 = F(2k+1) now
        // Below description is for "if (n & i)".
        // if i is 1000000, then do 1100100 & 1000000. Finally, I would get
        // true. if i is 0100000, then do 1100100 & 0100000. Finally, I would
        // get true. if i is 0000000, then do 1100100 & 0000000. Finally, I
        // would get false. if i is 0000000, then do 1100100 & 0000000. Finally,
        // I would get false. if i is 0000100, then do 1100100 & 0000100.
        // Finally, I would get true. if i is 1000000, then do 1100100 &
        // 1000000. Finally, I would get false.
        if (n & i) {             // odd
            bn_swap(f1, f2);     // f1 = F(2k+1)
            bn_add(f1, f2, f2);  // f2 = F(2k+2)
        }
        struct bn_hashtable *hash_t_loop =
            kmalloc(sizeof(struct bn_hashtable), GFP_KERNEL);
        hash_t_loop->id = count;
        bn *tmp_loop = bn_alloc(1);
        bn_cpy(tmp_loop, dest);
        hash_t_loop->bn_object = tmp_loop;
        hash_add(htable, &hash_t_loop->node, hash_t_loop->id);
    }
    bn_free(f2);
    bn_free(k1);
    bn_free(k2);
}
static int fib_open(struct inode *inode, struct file *file)
{
    // if (!mutex_trylock(&fib_mutex)) {
    //     printk(KERN_ALERT "fibdrv is in use");
    //     return -EBUSY;
    // }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    // mutex_unlock(&fib_mutex);
    return 0;
}

/*release hashtable*/
static void hashtable_release(void)
{
    struct bn_hashtable *pos = NULL;
    struct hlist_node *n = NULL;
    int bucket;

    for (bucket = 0; bucket < (1U << DEFAULT_HASHTABLE_LENGTH); ++bucket) {
        hlist_for_each_entry_safe(pos, n, &htable[bucket], node)
        {
            bn_free(pos->bn_object);
            hlist_del(&pos->node);
            kfree(pos);
        }
    }
}
/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    int len = 0;
    bn *fib = bn_alloc(1);
    bn_fib_doubling(fib, *offset);
    unsigned int key = 100;  // 以f(100)為例，這裡會設定 key 為 0, 1, 3, 6, 12,
                             // 25,50,100 ,驗證是否儲存正確
    struct bn_hashtable *hash_t_;
    hash_for_each_possible(htable, hash_t_, node, key)
    {
        if (hash_t_->id == key) {
            len = hash_t_->bn_object->size;
            copy_to_user(buf, hash_t_->bn_object->number,
                         sizeof(unsigned int) * len);
            printk(KERN_INFO "get target %d \n", key);
            bn_free(fib);
            return len;
        }
    }
    kfree(hash_t_);
    bn_free(fib);
    return len;
}
/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    // mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    hashtable_release();
    // mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include "../include/address_map_arm_vm.h"

/**  your part 2 kernel code here  **/
void *LW_virtual;
volatile int *KEY_ptr;
volatile int *SW_ptr;
#define SUCCESS 0
static int KEY_registered = 0;
static char KEY_msg[2];
static int SW_registered = 0;
static char SW_msg[5];


static int KEY_open (struct inode *, struct file *);
static int KEY_release (struct inode *, struct file *);
static ssize_t KEY_read (struct file *, char *, size_t, loff_t *);

static struct file_operations KEY_fops = {
    .owner = THIS_MODULE,
    .read = KEY_read,
    .open = KEY_open,
    .release = KEY_release
};

static struct miscdevice KEY = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "KEY",
    .fops = &KEY_fops,
    .mode = 0666
};


static int KEY_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}


static int KEY_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t KEY_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    sprintf (KEY_msg, "%x\n", *(KEY_ptr+0x3));
    
    size_t bytes;
    bytes = strlen (KEY_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &KEY_msg, bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    *(KEY_ptr+3) = 0xF;
    return bytes;
    
}

static int SW_open (struct inode *, struct file *);
static int SW_release (struct inode *, struct file *);
static ssize_t SW_read (struct file *, char *, size_t, loff_t *);

static struct file_operations SW_fops = {
    .owner = THIS_MODULE,
    .read = SW_read,
    .open = SW_open,
    .release = SW_release
};

static struct miscdevice SW = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "SW",
    .fops = &SW_fops,
    .mode = 0666
};


static int SW_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}


static int SW_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t SW_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    sprintf (SW_msg, "%.3x\n", *(SW_ptr));
    size_t bytes;
    bytes = strlen (SW_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &SW_msg, bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
    
}

static int __init KEY_SW_init(void)
{

    LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    KEY_ptr = (int *)(LW_virtual + KEY_BASE);
    SW_ptr = (int *)(LW_virtual + SW_BASE);
    *(KEY_ptr+3) = 0xF;

    int err = misc_register (&KEY);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "KEY");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "KEY");
        KEY_registered = 1;
    }

    int err2 = misc_register (&SW);
    if (err2 < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "SW");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "SW");
        SW_registered = 1;
    }
    
    return err;
    
}

static void __exit KEY_SW_exit(void)
{
    iounmap(LW_virtual);
    if (KEY_registered) {
        misc_deregister (&KEY);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "KEY");
    }
    if (SW_registered) {
        misc_deregister (&SW);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "SW");
    }
    return;
}


module_init(KEY_SW_init);
module_exit(KEY_SW_exit);
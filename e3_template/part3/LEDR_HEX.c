#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include "../include/address_map_arm_vm.h"


/**  your part 3 kernel code here  **/
void *LW_virtual;
volatile int *LEDR_ptr;
volatile int *HEX3_HEX0_ptr;
volatile int *HEX5_HEX4_ptr;
#define SUCCESS 0
static int LEDR_registered = 0;
static char LEDR_msg[3];
static int HEX_registered = 0;
static int HEX_msg[6];

const unsigned char seven_seg_digits_decode_gfedcba[10]= {
/*  0     1     2     3     4     5     6     7     8     9 */
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static int LEDR_open (struct inode *, struct file *);
static int LEDR_release (struct inode *, struct file *);
static ssize_t LEDR_write (struct file *, char *, size_t, loff_t *);

static struct file_operations LEDR_fops = {
    .owner = THIS_MODULE,
    .write = LEDR_write,
    .open = LEDR_open,
    .release = LEDR_release
};

static struct miscdevice LEDR = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "LEDR",
    .fops = &LEDR_fops,
    .mode = 0666
};


static int LEDR_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}


static int LEDR_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t LEDR_write(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = length;

    if (bytes > 3)    // can copy all at once, or not?
        bytes = 3;
    if (copy_from_user (LEDR_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    unsigned LEDR_num;
    sscanf (LEDR_msg, "%x", &LEDR_num);
    *LEDR_ptr = LEDR_num;
    
    return length;
    
}

static int HEX_open (struct inode *, struct file *);
static int HEX_release (struct inode *, struct file *);
static ssize_t HEX_write (struct file *, char *, size_t, loff_t *);

static struct file_operations HEX_fops = {
    .owner = THIS_MODULE,
    .write = HEX_write,
    .open = HEX_open,
    .release = HEX_release
};

static struct miscdevice HEX = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "HEX",
    .fops = &HEX_fops,
    .mode = 0666
};


static int HEX_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}


static int HEX_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t HEX_write(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = length;

    if (bytes > 6)    // can copy all at once, or not?
        bytes = 6;
    if (copy_from_user (HEX_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    unsigned HEX_num;
    sscanf (HEX_msg, "%d", &HEX_num);
    *HEX3_HEX0_ptr = seven_seg_digits_decode_gfedcba[(HEX_num/1000)%10]<<24 | seven_seg_digits_decode_gfedcba[(HEX_num/100)%10]<<16 | seven_seg_digits_decode_gfedcba[(HEX_num/10)%10]<<8 | seven_seg_digits_decode_gfedcba[HEX_num%10];
	*HEX5_HEX4_ptr = seven_seg_digits_decode_gfedcba[(HEX_num/100000)%10]<<8 | seven_seg_digits_decode_gfedcba[(HEX_num/10000)%10];
    return length;
    
    
}

static int __init LEDR_HEX_init(void)
{

    LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    LEDR_ptr = (int *)(LW_virtual + LEDR_BASE);
    HEX3_HEX0_ptr = (int *)(LW_virtual + HEX3_HEX0_BASE);
    HEX5_HEX4_ptr = (int *)(LW_virtual + HEX5_HEX4_BASE);
    *HEX3_HEX0_ptr = 0;
	*HEX5_HEX4_ptr = 0;
    *LEDR_ptr = 0;
    int err = misc_register (&LEDR);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "LEDR");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "LEDR");
        LEDR_registered = 1;
    }

    int err2 = misc_register (&HEX);
    if (err2 < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", "HEX");
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", "HEX");
        HEX_registered = 1;
    }
    
    return err;
    
}

static void __exit LEDR_HEX_exit(void)
{
    *HEX3_HEX0_ptr = 0;
	*HEX5_HEX4_ptr = 0;
    *LEDR_ptr = 0;
    iounmap(LW_virtual);
    if (LEDR_registered) {
        misc_deregister (&LEDR);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "LEDR");
    }
    if (HEX_registered) {
        misc_deregister (&HEX);
        printk (KERN_INFO "/dev/%s driver de-registered\n", "HEX");
    }
    return;
}


module_init(LEDR_HEX_init);
module_exit(LEDR_HEX_exit);
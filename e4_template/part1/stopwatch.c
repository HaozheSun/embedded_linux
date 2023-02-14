#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>               
#include <linux/init.h>             
#include <linux/module.h>           
#include <linux/miscdevice.h>       
#include <linux/uaccess.h>          
#include <asm/io.h>                 
#include "../include/address_map_arm.h"
#include "../include/interrupt_ID.h"


/**  your part 1 kernel code here  **/
static int stopwatch_open (struct inode *, struct file *);
static int stopwatch_release (struct inode *, struct file *);
static ssize_t stopwatch_read (struct file *, char *, size_t, loff_t *);
static ssize_t stopwatch_write(struct file *, const char *, size_t, loff_t *);

void* LW_virtual;
volatile int* TIMER0_ptr;
int ss = 59;
int mm = 59;
int dd = 99;

static struct file_operations stopwatch_fops = {
    .owner = THIS_MODULE,
    .read = stopwatch_read,
    .open = stopwatch_open,
    .release = stopwatch_release
};

#define SUCCESS 0
#define DEV_NAME "stopwatch"

static struct miscdevice stopwatch = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &stopwatch_fops,
    .mode = 0666
};
static int stopwatch_registered = 0;

#define MAX_SIZE 10                // we assume that no message will be longer than this
static char stopwatch_msg[MAX_SIZE];  // the character array that can be read or written
bool stop = false;
irq_handler_t irq_handler(int irq, void* dev_id, struct pt_regs* regs)
{
    *TIMER0_ptr &= 0UL << 0;

 
    if (stop == false) {
        dd--;
    }
    if (dd == -1) {
        dd = 99;
        ss--;
    }
    if (ss == -1) {
        ss = 59;
        mm--;
    }
    if (mm == -1) {
        mm = 0;
        ss = 0;
        dd = 0;
        *(TIMER0_ptr + 1) = 0x00;
        stop = true;
    }
    sprintf(stopwatch_msg, "%02d:%02d:%02d\n", mm, ss, dd);
    
    return (irq_handler_t)IRQ_HANDLED;
}

static int __init start_stopwatch(void)
{
    int err = misc_register (&stopwatch);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME);
        stopwatch_registered = 1;
    }
    
    LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    TIMER0_ptr = (int*) (LW_virtual + TIMER0_BASE);
    *(TIMER0_ptr + 2) = 0x4240;
    *(TIMER0_ptr + 3) = 0x0F;
    *(TIMER0_ptr + 1) = 0x07;
    sprintf(stopwatch_msg, "%02d:%02d:%02d\n", mm, ss, dd);
    int ret_val;
    ret_val = (int)request_irq(TIMER0_IRQ, (irq_handler_t)irq_handler, IRQF_SHARED, "timer0", (void*)(irq_handler));

    return err;
}

static void __exit stop_stopwatch(void)
{
    if (stopwatch_registered) {
        misc_deregister (&stopwatch);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME);
    }
    *(TIMER0_ptr + 1) = 0x00;
    free_irq(TIMER0_IRQ, (void*)(irq_handler));
    iounmap(LW_virtual);
}

/* Called when a process opens stopwatch */
static int stopwatch_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes stopwatch */
static int stopwatch_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from stopwatch. Provides character data from stopwatch_msg.
 * Returns, and sets *offset to, the number of bytes read. */
static ssize_t stopwatch_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = strlen (stopwatch_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &stopwatch_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}


module_init (start_stopwatch);
module_exit (stop_stopwatch);
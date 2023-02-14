#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include "../include/address_map_arm.h"
#include "../include/interrupt_ID.h"

/**  your part 2 kernel code here  **/
#include <linux/string.h>
static int stopwatch_open (struct inode *, struct file *);
static int stopwatch_release (struct inode *, struct file *);
static ssize_t stopwatch_read (struct file *, char *, size_t, loff_t *);
static ssize_t stopwatch_write(struct file *filp, char *buffer, size_t length, loff_t *offset);

void* LW_virtual;
volatile int* TIMER0_ptr;
volatile int *HEX3_HEX0_ptr;
volatile int *HEX5_HEX4_ptr;
int ss = 59;
int mm = 59;
int dd = 99;
const unsigned char seven_seg_digits_decode_gfedcba[10]= {
/*  0     1     2     3     4     5     6     7     8     9 */
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
static struct file_operations stopwatch_fops = {
    .owner = THIS_MODULE,
    .read = stopwatch_read,
    .write = stopwatch_write,
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
static char input_msg[MAX_SIZE];
bool stop = false;
bool display = false;
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
        stop = true;
    }
    sprintf(stopwatch_msg, "%02d:%02d:%02d\n", mm, ss, dd);
    if (display){
        *HEX3_HEX0_ptr = seven_seg_digits_decode_gfedcba[ss/10]<<24 | seven_seg_digits_decode_gfedcba[ss%10]<<16 | seven_seg_digits_decode_gfedcba[dd/10]<<8 | seven_seg_digits_decode_gfedcba[dd%10];
	    *HEX5_HEX4_ptr = seven_seg_digits_decode_gfedcba[mm/10]<<8 | seven_seg_digits_decode_gfedcba[mm%10];
    }
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
    HEX3_HEX0_ptr = (int*) (LW_virtual + HEX3_HEX0_BASE);
    HEX5_HEX4_ptr = (int*) (LW_virtual + HEX5_HEX4_BASE);
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
    *HEX3_HEX0_ptr = 0;
	*HEX5_HEX4_ptr = 0;
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

static ssize_t stopwatch_write(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (input_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    char input[bytes];
    sscanf(input_msg, "%s", input);
    if (strcmp(input, "run")==0){
        stop = false;
        *(TIMER0_ptr+1) = 0x7;
    }
    else if(strcmp(input, "stop")==0){
        stop = true;
        *(TIMER0_ptr+1) = 0xB;
    }
    else if(strcmp(input, "disp")==0){
        display = true;
    }
    else if(strcmp(input, "nodisp")==0){
        display = false;
        *HEX3_HEX0_ptr = 0;
	    *HEX5_HEX4_ptr = 0;
    }
    else{
        int i=0;
        int m,s,d;
        i = sscanf(input_msg, "%02d:%02d:%02d", &m, &s, &d);
        if (i==3){
            ss = s;
            mm = m;
            dd = d;
        }
    }
    // Note: we do NOT update *offset; we just copy the data into chardev_msg
    return bytes;
}

module_init (start_stopwatch);
module_exit (stop_stopwatch);
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "../include/address_map_arm.h"
#include "../include/interrupt_ID.h"

/**  your part 2 kernel code here  **/
#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev

static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);

void* LW_virtual;
volatile int* KEY_addr;
volatile int* SW_addr;
volatile int* HEX3_HEX0_addr;
volatile int* LEDR_addr;
volatile int* TIMER0_addr;
volatile int* GPIO0_addr;


static int signal_senerator_registered = 0;

const unsigned char seven_seg_digits_decode_gfedcba[10] = {
    /*  0     1     2     3     4     5     6     7     8     9 */
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static struct file_operations signal_senerator_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0
#define DEV_NAME "signal_senerator"

static struct miscdevice signal_senerator = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &signal_senerator_fops,
    .mode = 0666
};
void update_frenquency(void)
{
    int SW_val;
    int freq;

    SW_val = ((*(SW_addr) >> 6) * 10) + 10;
    freq = 100000000 / (2* SW_val);


    *(TIMER0_addr + 2) = freq&& 0xFFFF;
    *(TIMER0_addr + 3) = freq>>16;
    *(TIMER0_addr + 1) = 0x07;

}
irq_handler_t irq_handler(int irq, void* dev_id, struct pt_regs* regs)
{

    *(TIMER0_addr) = 0;
    *(GPIO0_addr) = *(GPIO0_addr) ^ 1;
    *LEDR_addr = *(SW_addr);
    update_frenquency();
    int SW_val;


    SW_val = ((*(SW_addr) >> 6) * 10) + 10;


    *HEX3_HEX0_addr = seven_seg_digits_decode_gfedcba[(SW_val % 10000) / 1000] << 24 | seven_seg_digits_decode_gfedcba[(SW_val % 1000) / 100] << 16 | seven_seg_digits_decode_gfedcba[(SW_val % 100) / 10] << 8 | seven_seg_digits_decode_gfedcba[SW_val % 10];

    return (irq_handler_t)IRQ_HANDLED;
}

static int __init init_signal_senerator(void) {

    int err = misc_register(&signal_senerator);
    if (err < 0) {
        printk(KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
    }
    else {
        printk(KERN_INFO "/dev/%s driver registered\n", DEV_NAME);
        signal_senerator_registered = 1;
    }
 int ret_val;
    ret_val = (int)request_irq(TIMER0_IRQ, (irq_handler_t)irq_handler, IRQF_SHARED, "timer0", (void*)(irq_handler));
    LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

    KEY_addr = (int*)(LW_virtual + KEY_BASE);
    SW_addr = (int*)(LW_virtual + SW_BASE);
    HEX3_HEX0_addr = (int*)(LW_virtual + HEX3_HEX0_BASE);
    LEDR_addr = (int*)(LW_virtual + LEDR_BASE);
    TIMER0_addr = (int*)(LW_virtual + TIMER0_BASE);
    GPIO0_addr = (int*)(LW_virtual + GPIO0_BASE);
	*(GPIO0_addr+1) = 1;
    update_frenquency();



   

    return err;

}
static void __exit stop_signal_senerator(void)
{
    if (signal_senerator_registered) {
        misc_deregister(&signal_senerator);
        printk(KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME);
    }
    *(TIMER0_addr + 1) = 0x00;
    *HEX3_HEX0_addr = 0;
    *LEDR_addr = 0;
    free_irq(TIMER0_IRQ, (void*)(irq_handler));
    iounmap(LW_virtual);
}

/* Called when a process opens stopwatch */
static int device_open(struct inode* inode, struct file* file)
{
    return SUCCESS;
}

/* Called when a process closes stopwatch */
static int device_release(struct inode* inode, struct file* file)
{
    return 0;
}

module_init(init_signal_senerator);
module_exit(stop_signal_senerator);
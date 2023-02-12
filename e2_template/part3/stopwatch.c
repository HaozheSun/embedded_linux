#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "../include/address_map_arm.h"
#include "../include/interrupt_ID.h"

/**  your part 3 kernel code here  **/
void* LW_virtual;
volatile int* KEY_ptr;
volatile int* SW_ptr;

volatile int* HEX3_HEX0_ptr;
volatile int* HEX5_HEX4_ptr;
volatile int* TIMER_ptr;
int ss = 59;
int mm = 59;
int dd = 99;

bool pause = false;
const unsigned char seven_seg_digits_decode_gfedcba[10] = {
    /*  0     1     2     3     4     5     6     7     8     9 */
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
irq_handler_t irq_handler(int irq, void* dev_id, struct pt_regs* regs)
{
    *TIMER_ptr |= 1UL << 0;

 
    if (pause == false) {
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
        mm = 59;
    }
    *HEX3_HEX0_ptr = seven_seg_digits_decode_gfedcba[ss / 10] << 24 | seven_seg_digits_decode_gfedcba[ss % 10] << 16 | seven_seg_digits_decode_gfedcba[dd / 10] << 8 | seven_seg_digits_decode_gfedcba[dd % 10];
    *HEX5_HEX4_ptr = seven_seg_digits_decode_gfedcba[mm / 10] << 8 | seven_seg_digits_decode_gfedcba[mm % 10];
    
  
 
    if (*(KEY_ptr + 3) == 0x1) {
        // printk(KERN_EMERG "Pasue\n");
        pause = !pause;
 
        *(KEY_ptr + 3) = 0xF;
    }
    if (*(KEY_ptr + 3) == 0x2) {
        if (pause == true) {
            if ((int)(*SW_ptr) >= 100) {
                dd = 99;
            }
            else {
                dd = (int)(*SW_ptr);
            }
        }
        
        printk(KERN_EMERG "%02d:%02d:%02d\n", mm, ss, dd);
       
   
        *(KEY_ptr + 3) = 0xF;
    }

    if (*(KEY_ptr + 3) == 0x4) {
        if (pause == true) {
            if ((int)(*SW_ptr) >= 60) {
                ss = 59;
            }
            else {
                ss = (int)(*SW_ptr);
            }
        }
      
        *(KEY_ptr + 3) = 0xF;
    }

    if (*(KEY_ptr + 3) == 0x8) {
        if (pause == true) {
            if ((int)(*SW_ptr) >= 60) {
                mm = 59;
            }
            else {
                mm = (int)(*SW_ptr);
            }
        }

 
        *(KEY_ptr + 3) = 0xF;
    }
    *(KEY_ptr + 3) = 0xF;
    return (irq_handler_t)IRQ_HANDLED;
}

static int __init clock_init(void)
{

    LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    KEY_ptr = (int*)(LW_virtual + KEY_BASE);
    SW_ptr = (int*)(LW_virtual + SW_BASE);

    HEX3_HEX0_ptr = (int*)(LW_virtual + HEX3_HEX0_BASE);
    HEX5_HEX4_ptr = (int*)(LW_virtual + HEX5_HEX4_BASE);
    TIMER_ptr = (int*)(LW_virtual + TIMER0_BASE);
    *(TIMER_ptr + 2) = 0x4240;
    *(TIMER_ptr + 3) = 0x0F;
    *(TIMER_ptr + 1) = 0x07;
    *(KEY_ptr + 3) = 0xF;
  
    int ret_val;
    ret_val = (int)request_irq(TIMER0_IRQ, (irq_handler_t)irq_handler, IRQF_SHARED, "timer0", (void*)(irq_handler));

    return 0;
}

static void __exit clock_exit(void)
{
    *HEX3_HEX0_ptr = 0;
    *HEX5_HEX4_ptr = 0;

    *(TIMER_ptr + 1) = 0x00;
    free_irq(TIMER0_IRQ, (void*)(irq_handler));
    iounmap(LW_virtual);
    return;
}


module_init(clock_init);
module_exit(clock_exit);

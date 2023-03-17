#include <linux/fs.h> // struct file, struct file_operations
#include <linux/init.h> // for __init, see code
#include <linux/module.h> // for module init and exit macros
#include <linux/miscdevice.h> // for misc_device_register and struct miscdev
#include <linux/uaccess.h> // for copy_to_user, see code
#include <linux/string.h>
#include <asm/io.h> // for mmap
#include "../include/address_map_arm.h"


/**  your part 1 kernel code here  **/
void * LW_virtual; // Lightweight bridge base address
volatile int * pixel_ctrl_ptr; // virtual address for the pixel buffer controller
volatile int * SDRAM_virtual;
volatile int * FPGA_ONCHIP_virtual;
int pixel_buffer;
int back_buffer;
int resolution_x, resolution_y; // screen resolution
#define ABS(x)				(((x) > 0) ? (x) : -(x))
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations video_fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0
#define DEV_NAME "video"

static struct miscdevice video = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &video_fops,
    .mode = 0666
};
static int video_registered = 0;

#define MAX_SIZE 256                // we assume that no message will be longer than this
static char video_msg[MAX_SIZE];  // the character array that can be read
static char input_msg[MAX_SIZE];

void swapint(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void plot_pixel(int x, int y, short int color) {
    *(short int *)(back_buffer + (y << 10) + (x << 1)) = color;
}

void get_screen_specs(volatile int * pixel_ctrl_ptr) {
    
    resolution_x = *(pixel_ctrl_ptr + 2) & 0xFFFF;
    resolution_y = *(pixel_ctrl_ptr + 2)>>16 & 0xFFFF;
}

void clear_screen(void) {
    int x, y;
    for (y = 0; y < resolution_y; y++) {
        for (x = 0; x < resolution_x; x++) {
            plot_pixel(x, y, 0);
        }
    }
}



void plot_line(int x0, int y0, int x1, int y1, short int color) {
    int is_steep = (ABS(y1 - y0) > ABS(x1 - x0));
    if (is_steep) {
        swapint(&x0, &y0);
        swapint(&x1, &y1);
    }
    if (x0>x1){
        swapint(&x0, &x1);
        swapint(&y0, &y1);
    }
    int dx = x1 - x0;
    int dy = ABS(y1 - y0);
    int error = -(dx / 2);
    int y = y0;
    int y_step = (y0 < y1) ? 1 : -1;
    int x = x0;
    for (x = x0; x <= x1; x++) {
        if (is_steep) {
            plot_pixel(y, x, color);
        } else {
            plot_pixel(x, y, color);
        }
        error += dy;
        if (error >= 0) {
            y += y_step;
            error -= dx;
        }
    }
}

void wait_for_vsync(volatile int * pixel_ctrl_ptr) {
    volatile int status;
    *pixel_ctrl_ptr = 1;
    status = *(pixel_ctrl_ptr + 3);
    while ((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }
    if (*(pixel_ctrl_ptr + 1) == SDRAM_BASE){
        back_buffer = (int) SDRAM_virtual;
    }
    else{
        back_buffer = (int) FPGA_ONCHIP_virtual;
    }
}
static int __init start_video(void)
{
    int err = misc_register (&video);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME);
        video_registered = 1;
    }
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    if (LW_virtual == 0)
        {printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
    }
    pixel_ctrl_ptr = LW_virtual + PIXEL_BUF_CTRL_BASE;
    get_screen_specs(pixel_ctrl_ptr);

    sprintf(video_msg, "%.3d %.3d", resolution_x, resolution_y);

    SDRAM_virtual = ioremap_nocache (SDRAM_BASE, SDRAM_SPAN);
    if (SDRAM_virtual == 0)
        {printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");}

    FPGA_ONCHIP_virtual = ioremap_nocache (FPGA_ONCHIP_BASE, FPGA_ONCHIP_SPAN);
    if (FPGA_ONCHIP_virtual == 0)
        {printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");}
    
    *(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE;
    back_buffer = (int)FPGA_ONCHIP_virtual;
    clear_screen ();
    wait_for_vsync (pixel_ctrl_ptr);
    //*(pixel_ctrl_ptr + 1) = SDRAM_BASE;
    //back_buffer = (int)SDRAM_virtual;
    //clear_screen ();
    return err;
}

static void __exit stop_video(void)
{
    iounmap (LW_virtual);
    iounmap (SDRAM_virtual);
    iounmap (FPGA_ONCHIP_virtual);
    if (video_registered) {
        misc_deregister (&video);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME);
    }
}

/* Called when a process opens video */
static int device_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes video */
static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from video. Provides character data from video_msg.
 * Returns, and sets *offset to, the number of bytes read. */
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = strlen (video_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &video_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}

/* Called when a process writes to video. Stores the data received into video_msg, and 
 * returns the number of bytes stored. */
static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = length;
    int error = 0;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (input_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    input_msg[bytes] = '\0';    // NULL terminate
    // Note: we do NOT update *offset; we just copy the data into input_msg
    char command[bytes];
    int i=0;
    i = sscanf(input_msg, "%s",command);
    int x1,x2,y1,y2,c;
    if (strcmp(command, "pixel") == 0) {
        i = sscanf(input_msg, "%s %03d,%03d %04x",command, &x1, &y1, &c);
        if (i == 4) {
            plot_pixel(x1, y1, c);
        }
        else {
            error = 1;
        }
    }
    else  if (strcmp(command, "line") == 0) {
        
        i = sscanf(input_msg, "%s %03d,%03d %03d,%03d %04x",command, &x1, &y1, &x2, &y2, &c);
        if (i == 6) {
            plot_line(x1, y1, x2, y2, c);
        }
        else {
            error = 1;
        }
    }
    else if (strcmp(command, "clear") == 0) {
        clear_screen();
    }
    else if (strcmp(command, "sync") == 0) {
        wait_for_vsync(pixel_ctrl_ptr);
    }
    else {
        if (strcmp(command, "--") != 0) {
            printk(KERN_ERR "Error \n");
        }
        printk(KERN_ERR "Usage: -- \n");
        printk(KERN_ERR "Usage: clear\n");
        printk(KERN_ERR "Usage: sync\n");
        printk(KERN_ERR "       pixel X,Y color\n");
        printk(KERN_ERR "       line X1,Y1 X2,Y2 color\n");
        printk(KERN_ERR "Notes: X1,Y1,X2,Y2 are integers, color is 4-digit hex value\n");

    }

    if (error == 1) {
        printk(KERN_ERR "Error \n");
        printk(KERN_ERR "Usage: -- \n");
        printk(KERN_ERR "Usage: clear\n");
        printk(KERN_ERR "Usage: sync\n");
        printk(KERN_ERR "       pixel X,Y color\n");
        printk(KERN_ERR "       line X1,Y1 X2,Y2 color\n");
        printk(KERN_ERR "Notes: X1,Y1,X2,Y2 are integers, color is 4-digit hex value\n");
    }
    return bytes;
}

MODULE_LICENSE("GPL");
module_init (start_video);
module_exit (stop_video);

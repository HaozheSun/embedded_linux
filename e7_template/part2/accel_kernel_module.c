#include <linux/kernel.h>
#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap

#include "../include/address_map_arm.h"
#include "../include/ADXL345.h"

// Declare global variables needed to use the accelerometer
volatile unsigned int * I2C0_ptr; // virtual address for I2C communication
volatile unsigned int * SYSMGR_ptr; // virtual address for System Manager communication


/**  implement your part 2 driver here  **/

static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations accel_fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0
#define DEV_NAME "accel"

static struct miscdevice accel = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &accel_fops,
    .mode = 0666
};
static int accel_registered = 0;

#define MAX_SIZE 256                // we assume that no message will be longer than this
static char accel_msg[MAX_SIZE]; 
static char input_msg[MAX_SIZE];
uint8_t devid;
int16_t mg_per_lsb = 31;
int16_t XYZ[3]={0,0,0};

void ADXL345_TAP(void)
{
    //Tap threshold set at 3g
    ADXL345_REG_WRITE(ADXL345_REG_THRESH_TAP, 0x2F);

    //Tap duration set at 0.02s
    ADXL345_REG_WRITE(ADXL345_REG_DURATION, 0x1F);

    //Tap latency set at 0.02s
    ADXL345_REG_WRITE(ADXL345_REG_LATENCY, 0xF);

    //Tap window set at 0.3s
    ADXL345_REG_WRITE(ADXL345_REG_WINDOW, 0xEF);

    //Enable tap in axes
    ADXL345_REG_WRITE(ADXL345_REG_TAP_AXES, 0x1);


    ADXL345_REG_WRITE(ADXL345_REG_INT_ENABLE, XL345_SINGLETAP | XL345_DOUBLETAP);
}


static int __init start_accel(void) {
    int err = misc_register (&accel);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME);
        accel_registered = 1;
    }



    /**  Note: include this code in your __init function  **/
    I2C0_ptr = ioremap_nocache (I2C0_BASE, I2C0_SPAN);
    SYSMGR_ptr = ioremap_nocache (SYSMGR_BASE, SYSMGR_SPAN);

    if ((I2C0_ptr == NULL) || (SYSMGR_ptr == NULL))
        printk (KERN_ERR "Error: ioremap_nocache returned NULL!\n");

    pass_addrs((unsigned int*) SYSMGR_ptr, (unsigned int*) I2C0_ptr);
    Pinmux_Config ();
    I2C0_Init ();
    ADXL345_Init ();
    ADXL345_REG_READ(0x00, &devid);
    ADXL345_TAP();
    return 0;

}

static void __exit stop_accel(void) {


    /**  Note: include this code in your __exit function  **/
    iounmap (I2C0_ptr);
    iounmap (SYSMGR_ptr);
    if (accel_registered) {
        misc_deregister (&accel);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME);
    }

}

static int device_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes accel */
static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from accel. Provides character data from accel_msg.
 * Returns, and sets *offset to, the number of bytes read. */
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    if (ADXL345_IsDataReady()){
        ADXL345_XYZ_Read(XYZ);
        sprintf(accel_msg, "1 %4d %4d %4d %2d\n" , XYZ[0], XYZ[1], XYZ[2], mg_per_lsb);
    }
    else{
        sprintf(accel_msg, "0 %4d %4d %4d %2d\n" , XYZ[0], XYZ[1], XYZ[2], mg_per_lsb);
    }
    accel_msg[strlen(accel_msg)] = '\0';
    bytes = strlen (accel_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &accel_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}

/* Called when a process writes to accel. Stores the data received into accel_msg, and 
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
    if (strcmp(command, "device") == 0){
        ADXL345_REG_READ(0x00, &devid);
        printk(KERN_ERR "Device: %u", devid);
    }
    else if (strcmp(command, "init") == 0){
        ADXL345_Init ();
    }
    else if (strcmp(command, "calibrate") == 0){
        //execute calibration routine
        ADXL345_Calibrate();
    }
    else if (strcmp(command, "format") == 0){
        int F, G;
        i = sscanf(input_msg, "%s %d %d",command, &F, &G);
        if (i != 3){
            printk(KERN_ERR "Invalid command. Type '--' for a list of commands.\n");
        }
        else{
            //need to set G
///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
            if (F == 0){
                ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_10BIT | );
            }
            else if (F == 1){
                ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_FULL_RESOLUTION |);
            }
////////////////////////////////////////////////////////////////////////////////////////////         
//////////////////////////////////////////////////////////////////////////////////////////////          
        }
    }
    else if (strcmp(command, "rate") == 0){
        float R;
        i = sscanf(input_msg, "%s %f",command, &R);
        if (i != 2){
            printk(KERN_ERR "Invalid command. Type '--' for a list of commands.\n");
        }
        else{
            if (R==1){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_1_56);
            }
            else if (R==3){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_3_13);
            }
            else if (R==6){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_6_25);
            }
            else if (R==12){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_12_5);
            }
            else if (R==25){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_25);
            }
            else if (R==50){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_50);
            }
            else if (R==100){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_100);
            }
            else if (R==200){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_200);
            }
            else if (R==400){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_400);
            }
            else if (R==800){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_800);
            }
            else if (R==1600){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_1600);
            }
            else if (R==3200){
                ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_3200);
            }
            else{
                printk(KERN_ERR "Invalid command. Type '--' for a list of commands.\n");
            }
        }
    }
    else if (strcmp(command, "--") == 0){
        printk(KERN_ERR "Commands:\n");
        printk(KERN_ERR "device: returns the device ID\n");
        printk(KERN_ERR "init: initializes the accelerometer\n");
        printk(KERN_ERR "calibrate: calibrates the accelerometer\n");
        printk(KERN_ERR "format F G: sets the data format to fixed 10-bit resolution (F=0) or full resolution(F=1), with range G=+/-2,4,8,16g\n");
        printk(KERN_ERR "rate R: sets the output data rate to R Hz, R is one of: 1, 3, 6, 12, 25, 50, 100, 200, 400, 800, 1600, 3200\n");
    }
    else{
        printk(KERN_ERR "Invalid command. Type '--' for a list of commands.\n");
        
    }
    return bytes;
}

MODULE_LICENSE("GPL");
module_init (start_accel);
module_exit (stop_accel);
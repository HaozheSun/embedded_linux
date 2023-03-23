#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "ADXL345.h"
#include "accel_wrappers.h"


/**             your part 2 user code here                   **/
/**  hint: you can call functions from ../accel_wrappers.c   **/
int running = 1;
void sig_handler(int signo)
{
    if (signo == SIGINT)
        running = 0;
}
int main(int argc, char *argv[])
{
    accel_open();
    
    int ready, x, y, z, mg_per_lsb;
    int i = 0;
    
    while(running)
    {
        signal(SIGINT, sig_handler);
        if(accel_read(&ready, &x, &y, &z, &mg_per_lsb))
        {
            if(ready)
            {
                printf("x: %d, y: %d, z: %d, mg_per_lsb: %d\n", x, y, z, mg_per_lsb);
            }
        }
    }

    accel_close();
    return 0;
}
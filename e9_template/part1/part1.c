#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "physical.h"
#include "address_map_arm.h"

/**  your part 1 user code here  **/
static volatile sig_atomic_t stop;
void catchSIGINT(int signum) {
    stop = 1;
}

int main(void) {
    int fd = -1;
    void* LW_virtual;
    volatile int* adc_ptr;
    fd = open_physical(fd);
    LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    adc_ptr = (int*)(LW_virtual + ADC_BASE);


    *(adc_ptr + 1) = 1;//active auto-update mode
    while (!stop) {
        signal(SIGINT, catchSIGINT);
        while ((*adc_ptr & 0x8000) == 0) {

        }
        float sample;
        sample = *adc_ptr & 0xFFF;
        sample = sample * 5.0/4095.0;

        printf("%.1f v \n", sample);
    }


    unmap_physical(LW_virtual, LW_BRIDGE_SPAN);
    close_physical(fd);

    return 0;
}

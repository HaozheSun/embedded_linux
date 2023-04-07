#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <physical.h>
#include <linux/input.h>
#include <pthread.h>
#include "address_map_arm.h"
#include "defines.h"

/**  your part 1 user code here  **/


int main(void){
    int fd=-1;
    void *LW_virtual;
    volatile int *audio_virtule;
    fd = open_physical (fd);
	LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    audio_virtule = (int *)(LW_virtual + AUDIO_BASE);
    float notes[13] = {MIDC, DFLAT, DNAT, EFLAT, ENAT, FNAT, GFLAT, GNAT, AFLAT, ANAT, BFLAT, BNAT, HIC};
    *audio_virtule |= 0x8;
    *audio_virtule &= ~0xB;
    int i=0;
    int j=0;
    for (i=0; i<13; i++){
        for (j=0; j<2400; j++){
            while ((((*(audio_virtule + 1) >> 16) & 0xFF) == 0x00) || (((*(audio_virtule + 1) >> 24) & 0xFF) == 0x00));
            *(audio_virtule + 2) = (int)MAX_VOLUME * sin(j * notes[i]);
            *(audio_virtule + 3) = (int)MAX_VOLUME * sin(j * notes[i]);
            
        }
    }
    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);

    return 0;
}

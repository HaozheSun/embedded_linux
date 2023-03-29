#define _GNU_SOURCE
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
#include "address_map_arm.h"
#include "defines.h"

/**  your part 2 user code here  **/


int main(int argc, char *argv[]){
    int fd=-1;
    void *LW_virtual;
    volatile int *audio_virtule;
    fd = open_physical (fd);
	LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    audio_virtule = (int *)(LW_virtual + AUDIO_BASE);
    float notes[13] = {MIDC, DFLAT, DNAT, EFLAT, ENAT, FNAT, GFLAT, GNAT, AFLAT, ANAT, BFLAT, BNAT, HIC};
    *audio_virtule |= 0x8;
    *audio_virtule &= ~0xB;

    char * input;
    input = argv[1];
    int i=0;
    int node = 0;
    int mixTone[13];
    if (strlen(input) != 13){
        printf("The input is wrong, plaese input 13 0/1s");
        return 0;
    }
    for (i = 0; i < strlen(input); i++){
        if (input[i] == '1'){
            node+=1;
            mixTone[i] = 1;
        }
        else{
            mixTone[i] = 0;
        }
    }
    int volume=MAX_VOLUME/node;
    int j=0;
    int sound=0;
    for (j=0; j<8000; j++){
        while ((((*(audio_virtule + 1) >> 16) & 0xFF) == 0x00) || (((*(audio_virtule + 1) >> 24) & 0xFF) == 0x00));
        sound=0;
        for (i=0; i<13; i++){
            if (mixTone[i] == 1){
                sound+=(int)volume * sin(j * notes[i]);
            }
        }
        *(audio_virtule + 2) = sound;
        *(audio_virtule + 3) = sound;
        
    }
    
    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);

    return 0;
}
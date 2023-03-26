#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include "../include/physical.h"
#include "../include/address_map_arm.h"

#define Clow_freq 261.626
#define Csharp_freq 277.183
#define D_freq 293.665
#define Dsharp_freq 311.127
#define E_freq 329.628
#define F_freq 349.228
#define Fsharp_freq 369.994
#define G_freq 391.995
#define Gsharp_freq 415.305
#define A_freq 440.000
#define Asharp_freq 466.164
#define B_freq 493.883
#define C_freq 523.251
#define MAX_VOLUME 0x7fffffff
#define PI 3.1415926
int main(void){
    int fd=-1;
    void *LW_virtual;
    volatile int *audio_virtule;
    fd = open_physical (fd);
	LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    audio_virtule = (int *)(LW_virtual + AUDIO_BASE);
    float notes[13] = {Clow_freq, Csharp_freq, D_freq, Dsharp_freq, 
                        E_freq, F_freq, Fsharp_freq, G_freq, Gsharp_freq,
                        A_freq, Asharp_freq, B_freq, C_freq};
    *audio_virtule |= 0x8;
    *audio_virtule &= ~0xB;
    int i=0;
    int j=0;
    for (i=0; i<13; i++){
        for (j=0; j<2400; j++){
            while ((((*(audio_virtule + 1) >> 16) & 0xFF) == 0x00) || (((*(audio_virtule + 1) >> 24) & 0xFF) == 0x00));
            *(audio_virtule + 2) = (int)MAX_VOLUME * sin(j * 2* PI * notes[i] / 8000);
            *(audio_virtule + 3) = (int)MAX_VOLUME * sin(j * 2* PI * notes[i] / 8000);
            
        }
    }
    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);

    return 0;
}

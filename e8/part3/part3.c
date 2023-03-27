#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <linux/input.h>
#include "../include/physical.h"
#include "../include/address_map_arm.h"
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

#define RELEASED 0
#define PRESSED 1
static const char *const press_type[2] = {"RELEASED", "PRESSED "};
int set_processor_affinity(unsigned int core) {
    cpu_set_t cpuset;
    pthread_t current_thread = pthread_self(); 
    
    if (core >= sysconf(_SC_NPROCESSORS_ONLN)){
        printf("CPU Core %d does not exist!\n", core);
        return -1;
    }
    // Zero out the cpuset mask
    CPU_ZERO(&cpuset);
    // Set the mask bit for specified core
    CPU_SET(core, &cpuset);
    
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset); 
}
char *Note[] = {"C#", "D#", " ", "F#", "G#", "A#", "C", "D", "E", "F", "G", "A", "B", "C"};
char ASCII[] = {'2',  '3',  '4', '5',  '6',  '7',  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I'};
float freq[] = {277.183, 311.127, 0, 369.994, 415.305, 466.164, 261.626, 293.665, 329.628, 349.228, 391.995, 440.000, 493.883, 523.251};
int tone_volume[14];
pthread_mutex_t volume_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_VOLUME 0x9D89D89
#define MIN_VOLUME 0x9D89D8
#define PI 3.1415926
static pthread_t tid1;
static volatile sig_atomic_t stop;
void catchSIGINT(int signum) {
	stop = 1;
}
void *audio_thread(void *audio_ptr) {
	set_processor_affinity(1);			// assign this thread to CPU 1
    volatile int *audio_virtule = (int *)audio_ptr;
    int sample = 0;
    int sound=0;
	while (1)
	{
		pthread_testcancel();			// exit if this thread has been cancelled
		while ((((*(audio_virtule + 1) >> 16) & 0xFF) == 0x00) || (((*(audio_virtule + 1) >> 24) & 0xFF) == 0x00));
        sound=0;
        int i=0;
        for (i=0; i<14; i++){
            sound+=(int)tone_volume[i] * sin(sample * 2* PI * freq[i] / 8000);
            pthread_mutex_lock (&volume_mutex);
			tone_volume[i] *= 0.9995;
            if (tone_volume[i] < MIN_VOLUME){
                tone_volume[i] = 0;
            }
		    pthread_mutex_unlock (&volume_mutex);
            
        }
        *(audio_virtule + 2) = sound;
        *(audio_virtule + 3) = sound;
        sample++;
        if (sample == 8000){
            sample = 0;
        }
		
	}
    
}

int main(int argc, char *argv[]){
    tone_volume[2] = 0;
    int fd=-1;
    void *LW_virtual;
    volatile int *audio_virtule;
    fd = open_physical (fd);
	LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    audio_virtule = (int *)(LW_virtual + AUDIO_BASE);

    *audio_virtule |= 0x8;
    *audio_virtule &= ~0xB;
    int err1;
    err1 = pthread_create (&tid1, NULL, &audio_thread, (void *)audio_virtule);

    set_processor_affinity(0);
    struct input_event ev;
	int fd_kb, event_size = sizeof (struct input_event), key;
	// set a default keyboard
	char *keyboard = "/dev/input/by-id/usb-Corsair_Corsair_Gaming_K55_RGB_Keyboard_AF7B95025F80CF26F5001C0640048000-event-kbd";
	
	// Open keyboard device
	if ((fd_kb = open (keyboard, O_RDONLY | O_NONBLOCK)) == -1) {
		printf ("Could not open %s\n", keyboard);
		return -1;
	}

    while (!stop) {
		// Read keyboard
        signal(SIGINT, catchSIGINT);
		if (read (fd_kb, &ev, event_size) < event_size)
			continue;
		if (ev.type == EV_KEY && (ev.value == PRESSED)) {
			key = (int) ev.code;
			if (key > 2 && key < 9){
                printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-3], 
					Note[key-3]);
                if (key != 5){
                    pthread_mutex_lock(&volume_mutex);
                    tone_volume[key-3] = MAX_VOLUME;
                    pthread_mutex_unlock(&volume_mutex);
                }

            }
				
			else if (key > 15 && key < 24){
                printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-10], 
					Note[key-10]);
                pthread_mutex_lock(&volume_mutex);
                tone_volume[key-10] = MAX_VOLUME;
                pthread_mutex_unlock(&volume_mutex);
            }
				
			else
				printf("You %s key code 0x%04x\n", press_type[ev.value], key);
		}
	}
	printf ("\nExiting program\n");
    pthread_cancel(tid1);
    pthread_join(tid1, NULL);
    close(fd_kb);
    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);

    return 0;
}

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
#include <pthread.h>
#include "address_map_arm.h"
#include "video.h"
#include "defines.h"
#include <intelfpgaup/video.h>

/**  your part 4 user code here  **/
#define RELEASED 0
#define PRESSED 1
static const char* const press_type[2] = { "RELEASED", "PRESSED " };
int set_processor_affinity(unsigned int core) {
	cpu_set_t cpuset;
	pthread_t current_thread = pthread_self();

	if (core >= sysconf(_SC_NPROCESSORS_ONLN)) {
		printf("CPU Core %d does not exist!\n", core);
		return -1;
	}
	// Zero out the cpuset mask
	CPU_ZERO(&cpuset);
	// Set the mask bit for specified core
	CPU_SET(core, &cpuset);

	return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}
char* Note[] = { "C#", "D#", " ", "F#", "G#", "A#", "C", "D", "E", "F", "G", "A", "B", "C" };
char ASCII[] = { '2',  '3',  '4', '5',  '6',  '7',  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I' };
float notes[13] = { MIDC, DFLAT, 0, DNAT, EFLAT, ENAT, FNAT, GFLAT, GNAT, AFLAT, ANAT, BFLAT, BNAT, HIC };
int tone_volume[14];

int press = 0;

pthread_mutex_t volume_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_VOLUME_Node 0x9D89D89
#define MIN_VOLUME 0x9D89D8
#define PI 3.1415926
static pthread_t tid1;
static pthread_t tid2;
static volatile sig_atomic_t stop;
void catchSIGINT(int signum) {
	stop = 1;
}
void* audio_thread(void* audio_ptr) {
	set_processor_affinity(1);			// assign this thread to CPU 1
	volatile int* audio_virtule = (int*)audio_ptr;
	int sample = 0;
	int sound = 0;

	while (1)
	{
		pthread_testcancel();			// exit if this thread has been cancelled
		while ((((*(audio_virtule + FIFOSPACE) >> 16) & 0xFF) < 0x15) && (((*(audio_virtule + FIFOSPACE) >> 24) & 0xFF) < 0x15));
		sound = 0;
		int i = 0;
		for (i = 0; i < 14; i++) {
			sound += (int)(tone_volume[i] * sin(sample * notes[i]));
			pthread_mutex_lock(&volume_mutex);
			tone_volume[i] *= 0.9995;
			if (tone_volume[i] < MIN_VOLUME) {
				tone_volume[i] = 0;
			}
			pthread_mutex_unlock(&volume_mutex);

		}
		*(audio_virtule + 2) = sound;
		*(audio_virtule + 3) = sound;
		sample++;


	}

}

void* video_thread() {
	set_processor_affinity(0);			// assign this thread to CPU 0
	int screen_x, screen_y;
	int char_x, char_y;
	video_open();
	video_read(&screen_x, &screen_y, &char_x, &char_y);
	// printf("x %d y %d cx %d cy %d \n", screen_x, screen_y, char_x, char_y);
	video_clear();
	video_show();

	int x;
	int y = 0;
	int y_1 = 0;
	int i;

	while (1)
	{
		pthread_testcancel();			// exit if this thread has been canceled

		if (press) {
			video_clear();

			for (x = 1; x < screen_x; x++) {
				for (i = 0; i < 14; i++) {
					y += (int)((tone_volume[i] / (int)(MAX_VOLUME_Node / (screen_y / 2))) * sin(x * notes[i]));
					//printf("tone: %d \n", tone_volume[i]);
					//printf("max-13 %d \n", (int)(MAX_VOLUME/13));

				}
				y += (screen_y / 2);

				if (y < 0) {
					y = 0;
				}
				if (y > screen_y - 1) {
					y = screen_y - 1;
				}

				video_line(x - 1, y_1, x, y, video_GREEN);

				y_1 = y;
				y = 0;

			}
			video_show();
			press = 0;
		}

	}

	video_close();


}

int main(int argc, char* argv[]) {
	tone_volume[2] = 0;
	int fd = -1;
	void* LW_virtual;
	volatile int* audio_virtule;
	fd = open_physical(fd);
	LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	audio_virtule = (int*)(LW_virtual + AUDIO_BASE);

	*audio_virtule |= 0x8;
	*audio_virtule &= ~0xB;
	int err1;
	err1 = pthread_create(&tid1, NULL, &audio_thread, (void*)audio_virtule);
	int err2;
	err2 = pthread_create(&tid2, NULL, &video_thread, NULL);

	set_processor_affinity(0);
	struct input_event ev;
	int fd_kb, event_size = sizeof(struct input_event), key;

	// set a default keyboard
	char* keyboard = "/dev/input/by-id/usb-Corsair_Corsair_Gaming_K55_RGB_Keyboard_AF7B95025F80CF26F5001C0640048000-event-kbd";

	if (argc != 2) {
		printf("Could not open keyboard \n");
		return -1;
	}
	else {
		keyboard = argv[1];
	}

	// Open keyboard device
	if ((fd_kb = open(keyboard, O_RDONLY | O_NONBLOCK)) == -1) {
		printf("Could not open %s\n", keyboard);
		return -1;
	}

	while (!stop) {
		// Read keyboard
		signal(SIGINT, catchSIGINT);
		if (read(fd_kb, &ev, event_size) < event_size)
			continue;
		if (ev.type == EV_KEY && (ev.value == PRESSED)) {
			key = (int)ev.code;
			if (key > 2 && key < 9) {
				press = 1;
				printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key - 3],
					Note[key - 3]);
				if (key != 5) {
					pthread_mutex_lock(&volume_mutex);
					tone_volume[key - 3] = MAX_VOLUME_Node;
					pthread_mutex_unlock(&volume_mutex);
				}

			}

			else if (key > 15 && key < 24) {
				press = 1;
				printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key - 10],
					Note[key - 10]);
				pthread_mutex_lock(&volume_mutex);
				tone_volume[key - 10] = MAX_VOLUME_Node;
				pthread_mutex_unlock(&volume_mutex);
			}

			else {

				printf("You %s key code 0x%04x\n", press_type[ev.value], key);
			}
		}

		if (ev.type == EV_KEY && (ev.value == RELEASED)) {
			key = (int)ev.code;

			if (key > 2 && key < 9) {
				press = 1;
			}

			else if (key > 15 && key < 24) {
				press = 1;
			}
		}

	}
	printf("\nExiting program\n");
	pthread_cancel(tid1);
	pthread_join(tid1, NULL);

	pthread_cancel(tid2);
	pthread_join(tid2, NULL);

	close(fd_kb);
	unmap_physical(LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);

	return 0;
}

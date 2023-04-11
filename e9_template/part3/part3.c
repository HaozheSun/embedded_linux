#include <stdio.h>
#include <signal.h>
#include <time.h>
#include "address_map_arm.h"
#include "physical.h"
#include "defines.h"
#include "video.h"
#include "SW.h"

/** timer data structures **/
struct itimerspec interval_timer_start = {
	.it_interval = {.tv_sec = 0,.tv_nsec = SAMPLING_PERIOD_NS},
	.it_value = {.tv_sec = 0,.tv_nsec = SAMPLING_PERIOD_NS} };

struct itimerspec interval_timer_stop = {
	.it_interval = {.tv_sec = 0,.tv_nsec = 0},
	.it_value = {.tv_sec = 0,.tv_nsec = 0} };

timer_t interval_timer_id;

volatile int* ADC_ptr;
volatile int* SW_ptr;
int samples_captured = 0;
unsigned int samples[320];
int ADC_sample;
int sample, sample_prev;

static volatile sig_atomic_t stop;
void catchSIGINT(int signum) {
	stop = 1;
}

/** timeout handler **/
void timeout_handler(int signum) {

	/** please complete this function **/

	samples[samples_captured] = (*ADC_ptr & 0xFFF);

	samples_captured++;

	if (samples_captured >= 320) {
		// turn off timer
		timer_settime(interval_timer_id, 0, &interval_timer_stop, NULL);
	}
}


int main(int argc, char* argv[])
{

	/**  please complete the main function **/
	int fd = -1;
	void* LW_virtual;

	fd = open_physical(fd);
	if (fd == -1) {
		return -1;
	}
	LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	ADC_ptr = (int*)(LW_virtual + ADC_BASE);
	SW_ptr = (int*)(LW_virtual + SW_BASE);

	*(ADC_ptr + 1) = 1;//active auto-update mode

	int screen_x, screen_y;
	int char_x, char_y;
	video_open();
	video_read(&screen_x, &screen_y, &char_x, &char_y);
	// printf("x %d y %d cx %d cy %d \n", screen_x, screen_y, char_x, char_y);
	video_clear();
	video_show();


	// Set up the signal handling (version provided in lab instructions)
	struct sigaction act;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	act.sa_flags = 0;
	act.sa_mask = set;
	act.sa_handler = &timeout_handler;
	sigaction(SIGALRM, &act, NULL);

	// set up signal handling (shorter version shown in lecture)
	signal(SIGALRM, timeout_handler);
	signal(SIGINT, catchSIGINT);


	// Create a monotonically increasing timer
	timer_create(CLOCK_MONOTONIC, NULL, &interval_timer_id);



	int trigger = 0;
	sample_prev = (*ADC_ptr & 0xFFF);


	while (!stop) {
		sample = (*ADC_ptr & 0xFFF);
		// printf("%.1f v \n", sample);

		if ((*SW_ptr & 0x1)&&(sample > sample_prev + 1000) && !trigger) {
			timer_settime(interval_timer_id, 0, &interval_timer_start, NULL);
			trigger = 1;
		}
		else if (!(*SW_ptr & 0x1) && (sample +1000 < sample_prev) && !trigger) {
			timer_settime(interval_timer_id, 0, &interval_timer_start, NULL);
			trigger = 1;
		}

		sample_prev = sample;




		
		int i;
		if (trigger) {
			while (samples_captured < 320) {

			}
			samples_captured = 0;
			video_clear();
			for (i = 0; i < (screen_x); i++) {

				if (i < screen_x - 1) {
					float test = samples[i] * 5.0 / 4095.0;

					float y1 = (samples[i] * 5.0 / 4095.0);
					float y2 = (samples[i + 1] * 5.0 / 4095.0);
					video_line(i, 239-(((y1 - 0) / (5 - 0)) * (239 - 10) + 10), i + 1, 239-(((y2 - 0) / (5 - 0)) * (239 - 10) + 10), video_GREEN);
				}

			}
			video_show();
			trigger = 0;
		}

		memset(samples, 0, sizeof(samples));


	}

	video_close();
	unmap_physical(LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);

}

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
#include <linux/input.h>
#include <pthread.h>
#include "address_map_arm.h"
#include "defines.h"
#include "stopwatch.h"
#include "video.h"
#include "KEY.h"
#include "HEX.h"
#include "LEDR.h"
#include "audio.h"


/**  your part 6 user code here  **/
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
void* audio_thread() {
    set_processor_affinity(1);			// assign this thread to CPU 1

    audio_open();
    audio_init();
    int sample = 0;
    int sound = 0;

   
    while (1)
    {
        pthread_testcancel();			// exit if this thread has been cancelled
        audio_wait_write();

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
        audio_write_left(sound);
        audio_write_right(sound);
        sample++;


    }
    audio_close();

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

int stopwatch_to_second(int MM, int SS, int DD) {
    int total_dsec = 0;
    total_dsec = MM * 60 + SS;
    total_dsec = total_dsec * 100 + DD;

    return total_dsec;
}

int main(int argc, char* argv[]) {
    tone_volume[2] = 0;



    int err1;
    err1 = pthread_create(&tid1, NULL, &audio_thread, NULL);
    int err2;
    err2 = pthread_create(&tid2, NULL, &video_thread, NULL);

    set_processor_affinity(0);
    struct input_event ev;
    int fd_kb, event_size = sizeof(struct input_event), key;
    int key_soc;
    KEY_open();

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
    stopwatch_open();
    LEDR_open();
    stopwatch_display();
    int press_count = 0;
    int release_count = 0;
    int press_time[500];
    int release_time[500];
    int release_key[500];
    int press_key[500];
    int m, s, d;
    int record = 0;
    memset(press_time, 0, sizeof(press_time));
    memset(release_time, 0, sizeof(release_time));
    memset(release_key, 0, sizeof(release_key));
    memset(press_key, 0, sizeof(press_key));


    while (!stop) {
        // Read keyboard
        signal(SIGINT, catchSIGINT);
        KEY_read(&key_soc);
        if (key_soc == 1) { //start timer or stop
            if (record == 0) {
                //stopwatch_stop();
                stopwatch_set(59, 59, 99);
                stopwatch_run();
                printf("Start %d", key_soc);
                record = 1;
                memset(press_time, 0, sizeof(press_time));
                memset(release_time, 0, sizeof(release_time));
                memset(release_key, 0, sizeof(release_key));
                memset(press_key, 0, sizeof(press_key));
                press_count = 0;
                release_count = 0;
                LEDR_set(0b1);
            }
            else if (record == 1) {
                record = 0;
                stopwatch_stop();
                LEDR_set(0);
            }
        }if (key_soc == 2 && record == 0) {
            LEDR_set(0b10);
            stopwatch_stop();
            stopwatch_set(59, 59, 99);
            stopwatch_run();

            int j = 0;
            int k = 0;


            while (1) {
                signal(SIGINT, catchSIGINT);
                if (press_time[j] == 0 && release_key[k] == 0) {
                    stopwatch_stop();
                    break;
                }

                stopwatch_read(&m, &s, &d);
                if (stopwatch_to_second(m, s, d) <= press_time[j]) {
                    press = 1;
                    pthread_mutex_lock(&volume_mutex);
                    if (press_key[j] > 2 && press_key[j] < 9) {
                        tone_volume[press_key[j] - 3] = MAX_VOLUME_Node;
                    }
                    if (press_key[j] > 15 && press_key[j] < 24) {
                        tone_volume[press_key[j] - 10] = MAX_VOLUME_Node;
                    }
                    pthread_mutex_unlock(&volume_mutex);
                    j++;
                }
                if (stopwatch_to_second(m, s, d) <= release_key[k]) {
                    press = 1;

                    k++;
                }
            }




        }
        if (read(fd_kb, &ev, event_size) < event_size)
            continue;
        if (ev.type == EV_KEY && (ev.value == PRESSED)) {
            key = (int)ev.code;
            if (key > 2 && key < 9) {
                press = 1;
                printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key - 3],
                    Note[key - 3]);
                if (key != 5) {
                    stopwatch_read(&m, &s, &d);
                    press_time[press_count] = stopwatch_to_second(m, s, d);
                    press_key[press_count] = key;
                    press_count++;

                    pthread_mutex_lock(&volume_mutex);
                    tone_volume[key - 3] = MAX_VOLUME_Node;
                    pthread_mutex_unlock(&volume_mutex);
                }

            }

            else if (key > 15 && key < 24) {
                press = 1;
                printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key - 10],
                    Note[key - 10]);

                stopwatch_read(&m, &s, &d);
                press_time[press_count] = stopwatch_to_second(m, s, d);
                press_key[press_count] = key;
                press_count++;

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
                if (key != 5) {
                    press = 1;
                    stopwatch_read(&m, &s, &d);
                    release_time[release_count] = stopwatch_to_second(m, s, d);
                    release_key[release_count] = key;
                    release_count++;
                }
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

    KEY_close();
    close(fd_kb);
    
 
    stopwatch_nodisplay();
    stopwatch_close();
    LEDR_set(0);
    LEDR_close();

    return 0;
}
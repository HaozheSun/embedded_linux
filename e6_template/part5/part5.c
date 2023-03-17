#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


/**  your part 5 user code here  **/
#define video_BYTES 8 // number of characters to read from /dev/video
int screen_x, screen_y;
int running = 1;
void sigint_handler(int sig) {
    running = 0;
}


void plot_animation(int x[5], int y[5], int c[5], int display, int number, int video_FD, char buffer[video_BYTES], char command[64]) {

    int color[6] = { 0xff0f,0x0fff,0xffff,0xf000, 0xf0ff, 0x0f0f };

    int k = 0;
    if (display == 1) {
        for (k = 0; k < number-1; k++) {
            sprintf(command, "line %d,%d %d,%d %X\n", x[k], y[k], x[k + 1], y[k + 1], color[c[k]]); // yellow
            write(video_FD, command, strlen(command));

        }

        sprintf(command, "line %d,%d %d,%d %X\n", x[number-1], y[number-1], x[0], y[0], color[c[number-1]]);
        write(video_FD, command, strlen(command));
    }

    for (k = 0; k < number; k++) {
        sprintf(command, "box %d,%d %d,%d %X\n", x[k], y[k], x[k] + 3, y[k] + 3, color[c[k]]);
        write(video_FD, command, strlen(command));
    }


}

int main(int argc, char* argv[])
{

    int video_FD; // file descriptor
    char buffer[video_BYTES]; // buffer for video char data
    char command[64]; // buffer for command data
    // Open the character device driver
    if ((video_FD = open("/dev/video", O_RDWR)) == -1) {
        printf("Error opening /dev/video: %s\n", strerror(errno));

    }

    int fd1 = -1;
    int fd2 = -1;
    signal(SIGINT, sigint_handler);
    fd1 = open("/dev/IntelFPGAUP/KEY", O_RDWR);
    if (fd1 == -1) {
        printf("Error opening /dev/IntelFPGAUP/KEY: %s\n", strerror(errno));
        return -1;
    }
    fd2 = open("/dev/IntelFPGAUP/SW", O_RDWR);
    if (fd2 == -1) {
        printf("Error opening /dev/IntelFPGAUP/SW: %s\n", strerror(errno));
        return -1;
    }



    // Read VGA screen size from the video driver
    read(video_FD, buffer, video_BYTES);
    sscanf(buffer, "%d %d", &screen_x, &screen_y);
    int i = 0;

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;


    srand(time(NULL));
    int x_cor[25];
    int y_cor[25];
    int c[25];
    int x_add[25];
    int y_add[25];
 
    int display = 1;
    int speed = 1;
    int number = 5;

    for (i = 0; i < 5; i++) {
        x_cor[i] = rand() % screen_x - 3;
        y_cor[i] = rand() % screen_y - 3;
        c[i] = rand() % 6;
        x_add[i] = (rand() % 2) * 2 - 1;
        y_add[i] = (rand() % 2) * 2 - 1;
    }


    char sw[3] = "000";
    /* Draw a few lines */
    while (running) {
        signal(SIGINT, sigint_handler);



        char key_read[2] = "00";
        read(fd1, &key_read, 1);
        read(fd2, &sw, 3);

        long sw_num;
        sw_num = strtol(sw, NULL, 16);
        long key;
        key = strtol(key_read, NULL, 16);

        if (sw_num != 0) {
            display = 0;
        }
        else {
            display = 1;
        }

        if (key == 1) {

            

            if (ts.tv_nsec > 8 * 1000000) {
                ts.tv_nsec = ts.tv_nsec / 2;
            }
            else {
                speed = speed * 2;
                if (speed > 8) {
                    speed = 8;
                }
            }

        }
        else if (key == 2) {

            if (speed > 1) {
                speed = speed / 2;
            }else if (ts.tv_nsec <= 8 * 1000000) {
                ts.tv_nsec = ts.tv_nsec * 2;  
            }
            


        }
        else if (key == 4) {


            number = number + 1;
            if (number > 25) {
                number = 25;
            }
            else {
                x_cor[number - 1] = rand() % screen_x - 3;
                y_cor[number - 1] = rand() % screen_x - 3;
                c[number - 1] = rand() % 6;
                x_add[number - 1] = (rand() % 2) * 2 - 1;
                y_add[number - 1] = (rand() % 2) * 2 - 1;
            }


        }
        else if (key == 8) {

            number = number - 1;
            if (number < 1) {
                number = 1;
            }
        }

        plot_animation(x_cor, y_cor, c, display, number, video_FD, buffer, command);


        for (i = 0; i < number; i++) {
            if (x_cor[i] == screen_x - 4 || x_cor[i] == 0) {
                x_add[i] = x_add[i] * -1;
            }
            if (y_cor[i] == screen_y - 4 || y_cor[i] == 0) {
                y_add[i] = y_add[i] * -1;
            }

        }

        for (i = 0; i < number; i++) {
            x_cor[i] = x_cor[i] + x_add[i]* speed;

            if (x_cor[i] >= screen_x - 4) {
                x_cor[i] = screen_x - 4;
            }
            if (x_cor[i] <= 0) {
                x_cor[i] = 0;
            }


            y_cor[i] = y_cor[i] + y_add[i] * speed;

            if (y_cor[i] >= screen_y - 4) {
                y_cor[i] = screen_y - 4;
            }
            if (y_cor[i] <= 0) {
                y_cor[i] = 0;
            }


        }
        nanosleep(&ts, NULL);
        sprintf(command, "sync");
        write(video_FD, command, strlen(command));
        sprintf(command, "clear");
        write(video_FD, command, strlen(command));


    }
    sprintf(command, "clear");
    write(video_FD, command, strlen(command));
    sprintf(command, "sync");
    write(video_FD, command, strlen(command));
    close(video_FD);
    return 0;
}

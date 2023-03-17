#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/**  your part 4 user code here  **/
#define video_BYTES 8 // number of characters to read from /dev/video
int screen_x, screen_y;
int running = 1;
void sigint_handler(int sig) {
    running = 0;
}


void plot_animation(int x[5], int y[5], int c[5], int video_FD, char buffer[video_BYTES],char command[64]) {

    int color[6] = { 0xff0f,0x0fff,0xffff,0xf000, 0xf0ff, 0x0f0f };

    int k = 0;
    for (k = 0; k < 4; k++) {
        sprintf(command, "line %d,%d %d,%d %X\n", x[k], y[k], x[k+1], y[k+1], color[c[k]]); // yellow
        write(video_FD, command, strlen(command));

    }
    
    sprintf(command, "line %d,%d %d,%d %X\n", x[4], y[4], x[0], y[0], color[c[4]]); 
    write(video_FD, command, strlen(command));

    for (k = 0; k < 5; k++) {
        sprintf(command, "box %d,%d %d,%d %X\n", x[k], y[k], x[k]+3, y[k]+3, color[c[k]]);
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
    // Read VGA screen size from the video driver
    read(video_FD, buffer, video_BYTES);
    sscanf(buffer, "%d %d", &screen_x, &screen_y);
    int i = 0;
   

    srand(time(NULL));
    int x_cor[5];
    int y_cor[5];
    int c[5];
    int x_add[5];
    int y_add[5];
 
    for (i = 0; i < 5; i++) {
        x_cor[i] = rand() % screen_x-3;
        y_cor[i] = rand() % screen_y-3;
        c[i] = rand() % 6;
        x_add[i] = (rand() % 2) * 2 - 1;
        y_add[i] = (rand() % 2) * 2 - 1;
    }
    /* Draw a few lines */
    while (running) {
        signal(SIGINT, sigint_handler);
        plot_animation(x_cor, y_cor , c, video_FD, buffer, command);
        sprintf(command, "sync");
        write(video_FD, command, strlen(command));
        sprintf(command, "clear");
        write(video_FD, command, strlen(command));

        for (i = 0; i < 5; i++) {
            if (x_cor[i] == screen_x-4 || x_cor[i] == 0) {
                x_add[i] = x_add[i] * -1;
            }
            if (y_cor[i] == screen_y-4 || y_cor[i] == 0) {
                y_add[i] = y_add[i] * -1;
            }

        }

        for (i = 0; i < 5; i++) {
            x_cor[i] = x_cor[i] + x_add[i];

            if (x_cor[i] >= screen_x-4) {
                x_cor[i] = screen_x-4;
            }
            if (x_cor[i] <= 0) {
                x_cor[i] = 0;
            }


            y_cor[i] = y_cor[i] + y_add[i];

            if (y_cor[i] >= screen_y-4) {
                y_cor[i] = screen_y-4;
            }
            if (y_cor[i] <= 0) {
                y_cor[i] = 0;
            }


        }


    }
    sprintf(command, "clear");
    write(video_FD, command, strlen(command));
    sprintf(command, "sync");
    write(video_FD, command, strlen(command));
    close(video_FD);
    return 0;
}

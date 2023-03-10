#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "defines.h"

/**  your part 5 user code here  **/

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void plot_pixel(int x, int y, char color, char c) {
    printf("\e[%2dm\e[%d;%dH%c", color, y, x, c);
    fflush(stdout);
}
void plot_line(int x0, int y0, int x1, int y1, char color, char c) {
    int is_steep = (abs(y1 - y0) > abs(x1 - x0));
    if (is_steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    int dx = x1 - x0;
    int dy = ABS(y1 - y0);
    int error = -(dx / 2);
    int y = y0;
    int y_step = (y0 < y1) ? 1 : -1;
    int x = x0;
    for (x = x0; x <= x1; x++) {
        if (is_steep) {
            plot_pixel(y, x, color, c);
        }
        else {
            plot_pixel(x, y, color, c);
        }
        error += dy;
        if (error >= 0) {
            y += y_step;
            error -= dx;
        }
    }
}
void plot_animation(int x[5], int y[5], int c[5],int display,int number) {

    int i;
    if (display == 1) {
        for (i = 0; i < number-1; i++) {

            plot_line(x[i], y[i], x[i+1], y[i+1], c[i], '*');
           
        }
        plot_line(x[number - 1], y[number - 1], x[0], y[0], c[number - 1], '*');
    }

    for (i = 0; i < number ; i++) {
        plot_pixel(x[i], y[i], c[i], 'X');

    }


}
int running = 1;
void sigint_handler(int sig) {
    running = 0;
}
int main(void)
{
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


    printf("\e[2J"); // clear the screen
    printf("\e[?25l"); // hide the cursor
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000000000;
    srand(time(NULL));

    int x_cor[20];
    int y_cor[20];
    int c[20];
    int x_add[20];
    int y_add[20];
    int i;
    int display = 1;
    double speed = 1;
    int number = 5;

    for (i = 0; i < 5; i++) {
        x_cor[i] = rand() % ((80 + 1) - 1) + 1;
        y_cor[i] = rand() % ((24 + 1) - 1) + 1;
        c[i] = rand() % ((37 + 1) - 31) + 31;
        x_add[i] = (rand() % 2) * 2 - 1;
        y_add[i] = (rand() % 2) * 2 - 1;
    }
    char sw[3] = "000";
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

            speed = speed * 2;
         
        }
        else if (key == 2) {

            speed = speed / 2;
        

        }
        else if (key == 4) {

         
            number = number + 1;
            if (number >  20) {
                number = 20;
            }
            else {
                x_cor[number-1] = rand() % ((80 + 1) - 1) + 1;
                y_cor[number - 1] = rand() % ((24 + 1) - 1) + 1;
                c[number - 1] = rand() % ((37 + 1) - 31) + 31;
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



        plot_animation(x_cor, y_cor, c, display, number);
        for (i = 0; i < number; i++) {
            if (x_cor[i] == 80 || x_cor[i] == 1) {
                x_add[i] = x_add[i] * -1;
            }
            if (y_cor[i] == 24 || y_cor[i] == 1) {
                y_add[i] = y_add[i] * -1;
            }

        }

        for (i = 0; i < number; i++) {
            x_cor[i] = x_cor[i] + x_add[i]* speed;

            if (x_cor[i] >= 80) {
                x_cor[i] = 80;
            }
            if (x_cor[i] <= 1) {
                x_cor[i] = 1;
            }


            y_cor[i] = y_cor[i] + y_add[i]* speed;

            if (y_cor[i] >= 24) {
                y_cor[i] = 24;
            }
            if (y_cor[i] <= 1) {
                y_cor[i] = 1;
            }


        }
        nanosleep(&ts, NULL);
        printf("\e[2J"); // clear the screen


    }


    printf("\e[2J"); // clear the screen
    printf("\e[%2dm", WHITE); // reset foreground color
    printf("\e[%d;%dH", 1, 1); // move cursor to upper left
    printf("\e[?25h"); // show the cursor
    fflush(stdout);
    close(fd1);
    close(fd2);
}

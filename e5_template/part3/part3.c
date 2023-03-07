#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "defines.h"

/**  your part 3 user code here  **/
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void plot_pixel(int x, int y, char color, char c) {
    printf ("\e[%2dm\e[%d;%dH%c", color, y, x, c);
    fflush (stdout);
}
void plot_line(int x0, int y0, int x1, int y1, char color, char c) {
    int is_steep = (abs(y1 - y0) > abs(x1 - x0));
    if (is_steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0>x1){
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
        } else {
            plot_pixel(x, y, color, c);
        }
        error += dy;
        if (error >= 0) {
            y += y_step;
            error -= dx;
        }
    }
}

int running = 1;
void sigint_handler(int sig) {
    running = 0;
}
int main(void)
{
    printf ("\e[2J"); // clear the screen
    printf ("\e[?25l"); // hide the cursor
    int y=1;
    int y_step = 1;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    while(running){
        signal(SIGINT, sigint_handler);
        plot_line(1, y, 80, y, CYAN, '*');
        nanosleep(&ts, NULL);
        printf ("\e[2J"); // clear the screen
        y += y_step;
        if (y == 24 || y == 1) {
            y_step = -y_step;
        }
    }
    
    printf ("\e[2J"); // clear the screen
    printf ("\e[%2dm", WHITE); // reset foreground color
    printf ("\e[%d;%dH", 1, 1); // move cursor to upper left
    printf ("\e[?25h"); // show the cursor
    fflush (stdout);
}

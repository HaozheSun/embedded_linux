#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "defines.h"

/**  your part 4 user code here  **/
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
void plot_animation(int x[5], int y[5], int c[5]) {



    plot_line(x[0], y[0], x[1], y[1], c[0], '*');
    plot_line(x[1], y[1], x[2], y[2], c[1], '*');
    plot_line(x[2], y[2], x[3], y[3], c[2], '*');
    plot_line(x[3], y[3], x[4], y[4], c[3], '*');
    plot_line(x[4], y[4], x[0], y[0], c[4], '*');

    plot_pixel(x[0], y[0], c[0], 'X');
    
    plot_pixel(x[1], y[1], c[1], 'X');
    
    plot_pixel(x[2], y[2], c[2], 'X');
    
    plot_pixel(x[3], y[3], c[3], 'X');
    
    plot_pixel(x[4], y[4], c[4], 'X');
   

}
int running = 1;
void sigint_handler(int sig) {
    running = 0;
}
int main(void)
{

    printf("\e[2J"); // clear the screen
    printf("\e[?25l"); // hide the cursor
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000000000;
    srand(time(NULL));

    int x_cor[5];
    int y_cor[5];
    int c[5];
    int x_add[5];
    int y_add[5];
    int i;
    for (i = 0; i < 5; i++) {
        x_cor[i] = rand() % ((80 + 1) - 1) + 1;
        y_cor[i] = rand() % ((24 + 1) - 1) + 1;
        c[i] = rand() % ((37 + 1) - 31) + 31;
        x_add[i] = (rand() % 2) * 2 - 1;
        y_add[i] = (rand() % 2) * 2 - 1;
    }

    while (running) {
        signal(SIGINT, sigint_handler);
        plot_animation(x_cor, y_cor,  c);


        for (i = 0; i < 5; i++) {
            if (x_cor[i] == 80 || x_cor[i] == 1) {
                x_add[i] = x_add[i] * -1;
            }
            if (y_cor[i] == 24 || y_cor[i] == 1) {
                y_add[i] = y_add[i] * -1;
            }

        }

        for ( i = 0; i < 5; i++) {
              x_cor[i] = x_cor[i] + x_add[i];

              if (x_cor[i] >= 80 ) {
                  x_cor[i] = 80;
              }
              if (x_cor[i] <= 1) {
                  x_cor[i] = 1;
              }


              y_cor[i] = y_cor[i] + y_add[i];

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
}

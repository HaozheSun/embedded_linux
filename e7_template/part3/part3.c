#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "ADXL345.h"
#include "accel_wrappers.h"
void plot_pixel(int, int, char, char);

/**             your part 3 user code here                   **/
/**  hint: you can call functions from ../accel_wrappers.c   **/
int running = 1;
void sig_handler(int signo)
{
    if (signo == SIGINT)
        running = 0;
}
int main(int argc, char* argv[])
{
    accel_open();

    int ready, x, y, z, mg_per_lsb;
    int i = 0;

    int x_point = 40;
    int y_point = 12;
    int av_x = 0;
    int av_y = 0;
    char c;
    printf("\e[2J"); // clear the screen
    printf("\e[?25l"); // hide the cursor
    while (running)
    {
        signal(SIGINT, sig_handler);
        if (accel_read(&ready, &x, &y, &z, &mg_per_lsb))
        {
            if (ready)
            {

                av_x = av_x * 0.9 + x * 0.1;
                av_y = av_y * 0.9 + y * 0.1;

                x_point = 40 + av_x;
                y_point = 12 + av_y;
                printf("\e[2J"); // clear the screen
                printf("\e[%2dm", 37);
                printf("\e[%d;%dH", 1, 1);
                printf("x: %d, y: %d, z: %d, mg_per_lsb: %d\n", x * mg_per_lsb, y * mg_per_lsb, z * mg_per_lsb, mg_per_lsb);
                plot_pixel(x_point, y_point, 32, 'o');
            }
        }
    }
  

    
    //plot_pixel(80, 24, 32, 'o');


    // wait for user to press return
    printf("\e[2J"); // clear the screen
    printf("\e[%2dm", 37); // reset foreground color
    printf("\e[%d;%dH", 1, 1); // move cursor to upper left
    printf("\e[?25h"); // show the cursor
    fflush(stdout);

  

    accel_close();
    return 0;
}

void plot_pixel(int x, int y, char color, char c) {
    printf("\e[%2dm\e[%d;%dH%c", color, y, x, c);
    fflush(stdout);
}


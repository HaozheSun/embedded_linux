#include <stdio.h>
#include "defines.h"

/**  your part 2 user code here  **/

//Bresenham’s line-drawing algorithm
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
    for (int x = x0; x <= x1; x++) {
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

int main(void)
{
    char c;
    int i;
    printf ("\e[2J"); // clear the screen
    printf ("\e[?25l"); // hide the cursor

    plot_line(1, 1, 80, 24, CYAN, '*');
    plot_line(80, 24, 1, 1, RED, '*');
    plot_line(40, 8, 40, 18, YELLOW, '*');
    plot_line(1, 24, 80, 1, GREEN, '*');
    plot_line(1, 24, 80, 12, BLUE, '*');
    plot_line(1, 24, 80, 24, MAGENTA, '*');
    c = getchar (); // wait for user to press return
    printf ("\e[2J"); // clear the screen
    printf ("\e[%2dm", WHITE); // reset foreground color
    printf ("\e[%d;%dH", 1, 1); // move cursor to upper left
    printf ("\e[?25h"); // show the cursor
    fflush (stdout);
}


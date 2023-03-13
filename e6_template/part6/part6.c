#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/**  your part 6 user code here  **/
#define video_BYTES 8 // number of characters to read from /dev/video
int screen_x, screen_y;
int running = 1;
int frames = 0;
void sigint_handler(int sig) {
    running = 0;
}
int main(int argc, char *argv[])
{

    int video_FD; // file descriptor
    char buffer[video_BYTES]; // buffer for video char data
    char command[64]; // buffer for command data
    // Open the character device driver
    if ((video_FD = open("/dev/video", O_RDWR)) == -1) {
    printf("Error opening /dev/video: %s\n", strerror(errno));
        
    }
    // Read VGA screen size from the video driver
    read(video_FD , buffer, video_BYTES);
    sscanf(buffer, "%d %d", &screen_x, &screen_y);
    int i=0;
    int dy=1;
    /* Draw a few lines */
    while (running){
        signal(SIGINT, sigint_handler);
        sprintf (command, "line %d,%d %d,%d %X\n", 0, i, screen_x - 1, i, 0xFFE0); // yellow
        write(video_FD, command, strlen(command));
        frames++;
        sprintf (command, "text 0,0 %d\n", frames);
        write (video_FD, command, strlen(command));
        sprintf(command, "sync");
        write(video_FD, command, strlen(command));
        sprintf(command, "clear");
        write(video_FD, command, strlen(command));
        
        i+=dy;
        if (i==screen_y-1 || i==0){
            dy=-dy;
        }
    }
    sprintf(command, "clear");
    write(video_FD, command, strlen(command));
	sprintf(command, "sync");
    write(video_FD, command, strlen(command));
    close (video_FD);
    return 0;
}

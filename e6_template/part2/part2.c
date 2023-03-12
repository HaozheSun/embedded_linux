#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

/**  your part 2 user code here  **/
#define video_BYTES 8 // number of characters to read from /dev/video
int screen_x, screen_y;
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
    /* Draw a few lines */
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, screen_x - 1, 0, 0xFFE0); // yellow
    write (video_FD, command, strlen(command));
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, (screen_x >> 1) - 1, 0, 0x07FF); // cyan
    write (video_FD, command, strlen(command)); 
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, (screen_x >> 2) - 1, 0, 0x07E0);
    write (video_FD, command, strlen(command));
    sprintf(command, "sync");
    write(video_FD, command, strlen(command));
    close (video_FD);
    return 0;
}

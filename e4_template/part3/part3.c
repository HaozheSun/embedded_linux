#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/**  your part 3 user code here  **/
#include <stdbool.h>
int running=1;
void intHandler(int dummy){
	running=0;
}
int main (void){
    bool stop = false;
    int fd1=-1;
    int fd2=-1;
    int fd3=-1;
    int fd4=-1;
    fd1 = open("/dev/KEY", O_RDWR);
    if (fd1 == -1){
        printf("Error opening /dev/KEY: %s\n", strerror(errno));
        return -1;
    }
    fd2 = open("/dev/SW", O_RDWR);
    if (fd2 == -1){
        printf("Error opening /dev/SW: %s\n", strerror(errno));
        return -1;
    }
    fd3 = open("/dev/LEDR", O_RDWR);
    if (fd3 == -1){
        printf("Error opening /dev/LEDR: %s\n", strerror(errno));
        return -1;
    }
    fd4 = open("/dev/stopwatch", O_RDWR);
    if (fd4 == -1){
        printf("Error opening /dev/stopwatch: %s\n", strerror(errno));
        return -1;
    }
    write(fd4, "disp", 4);
    
    char sw[3]="000";
    while(running){
        signal(SIGINT, intHandler);
        char key;
        read(fd1, &key, 1);
        read(fd2, &sw, 3);

        long sw_num;
        sw_num = strtol(sw, NULL, 16);
        char sw_num_char[3];
        sprintf(sw_num_char, "%.3x", sw_num);
        write(fd3, &sw_num_char, strlen(sw_num_char));
        int sw_decimal = 0;
        if (key!='0' && key!='1'){
            int i;
            int base = 1;
            
            for(i = 2; i >= 0; i--)
            {
                if(sw_num_char[i] >= '0' && sw_num_char[i] <= '9')
                {
                    sw_decimal += (sw_num_char[i] - 48) * base;
                    base *= 16;
                }
                else if(sw_num_char[i] >= 'A' && sw_num_char[i] <= 'F')
                {
                    sw_decimal += (sw_num_char[i] - 55) * base;
                    base *= 16;
                }
                else if(sw_num_char[i] >= 'a' && sw_num_char[i] <= 'f')
                {
                    sw_decimal += (sw_num_char[i] - 87) * base;
                    base *= 16;
                }
            }
        }
        
        
        if (key=='1'){
            
            if(!stop){
                
                write(fd4, "stop", 4);
                
            }
            else{
                
                write(fd4, "run", 3);
            }
            stop = !stop;
        }
        else if(key=='2'){
            
            if (sw_decimal > 99){
                sw_decimal = 99;
            }
            char sw_decimal_char[2];
            sprintf(sw_decimal_char, "%.2d", sw_decimal);
            char msg[8]="-1:-1:";
            strcat(msg,sw_decimal_char);
            write(fd4, &msg, strlen(msg));
            
        }
        else if(key=='4'){
            
            if (sw_decimal > 59){
                sw_decimal = 59;
            }
            char sw_decimal_char[2];
            sprintf(sw_decimal_char, "%.2d", sw_decimal);
            char msg[8]="-1:";
            strcat(msg,sw_decimal_char);
            strcat(msg,":-1");
            write(fd4, &msg, strlen(msg));
        }
        else if(key=='8'){
            
            if (sw_decimal > 59){
                sw_decimal = 59;
            }
            char sw_decimal_char[2];
            sprintf(sw_decimal_char, "%.2d", sw_decimal);
            char msg[8]="";
            strcat(msg,sw_decimal_char);
            strcat(msg,":-1:-1");
            write(fd4, &msg, strlen(msg));
        }
    }
    close(fd1);
    close(fd2);
    close(fd3);
    close(fd4);
    return 0;
}
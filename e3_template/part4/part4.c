#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/**  your part 4 user code here  **/
int running=1;
void intHandler(int dummy){
	running=0;
}
void main (void){
    int fd1=-1;
    int fd2=-1;
    int fd3=-1;
    int fd4=-1;
    fd1 = open("/dev/KEY", O_RDWR);
    fd2 = open("/dev/SW", O_RDWR);
    fd3 = open("/dev/LEDR", O_RDWR);
    fd4 = open("/dev/HEX", O_RDWR);
    if (fd1 == -1 || fd2 == -1 || fd3 == -1 || fd4 == -1){
        printf("Error opening /dev/: %s\n", strerror(errno));
        return -1;
    }
    
    int sum=0;
    
    char sw[3]="000";
    while(running){
        signal(SIGINT, intHandler);
        char key;
        read(fd1, &key, 1);
        read(fd2, &sw, 3);
        if (key!='0'){
            // printf("key pressed: %c \n", key);
            
            // printf("sw: %s\n", sw);
            long sw_num;
            sw_num = strtol(sw, NULL, 16);
            // printf("sw_num: %d\n", sw_num);
            int sw_decimal=0;
            
            char sw_num_char[3];
            sprintf(sw_num_char, "%.3x", sw_num);
            // printf("sw_num_char: %s\n", sw_num_char);

            
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
            // printf("sw_decimal: %d\n", sw_decimal);
            if (sw_decimal==0){
                sum=0;
            }
            sum+=sw_decimal;
            char sum_char[6];
            sprintf(sum_char, "%.6d", sum);
            // printf("sum: %d\n", sum);
            // printf("sum_char: %s\n", sum_char);

            write(fd3, &sw_num_char, strlen(sw_num_char));
            write(fd4, &sum_char, strlen(sum_char));
        }
        
    }
    close(fd1);
    close(fd2);
    close(fd3);
    close(fd4);
}
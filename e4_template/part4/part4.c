#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>


/**  your part 4 user code here  **/
#include <stdbool.h>
#include <stdbool.h>
int running = 1;
int setting = 1;
void intHandler(int dummy) {
    running = 0;
    setting = 0;
    fclose(stdin);
}
int main(void) {
    bool stop = false;
    int fd1 = -1;
    int fd2 = -1;
    int fd3 = -1;
    int fd4 = -1;
    signal(SIGINT, intHandler);

    fd1 = open("/dev/KEY", O_RDWR);
    if (fd1 == -1) {
        printf("Error opening /dev/KEY: %s\n", strerror(errno));
        return -1;
    }
    fd2 = open("/dev/SW", O_RDWR);
    if (fd2 == -1) {
        printf("Error opening /dev/SW: %s\n", strerror(errno));
        return -1;
    }
    fd3 = open("/dev/LEDR", O_RDWR);
    if (fd3 == -1) {
        printf("Error opening /dev/LEDR: %s\n", strerror(errno));
        return -1;
    }
    fd4 = open("/dev/stopwatch", O_RDWR);
    if (fd4 == -1) {
        printf("Error opening /dev/stopwatch: %s\n", strerror(errno));
        return -1;
    }


 
    char start[8] = "00:10:00";
    write(fd4, &start, strlen(start));
    write(fd4, "disp", 4);
    write(fd4, "stop", 4);

    char sw[3] = "000";
    while (setting) {
        
        char key;
        read(fd1, &key, 1);
        read(fd2, &sw, 3);

        long sw_num;
        sw_num = strtol(sw, NULL, 16);
        char sw_num_char[3];
        sprintf(sw_num_char, "%.3x", sw_num);
        write(fd3, &sw_num_char, strlen(sw_num_char));
        int sw_decimal = 0;
        if (key != '0' && key != '1') {
            int i;
            int base = 1;

            for (i = 2; i >= 0; i--)
            {
                if (sw_num_char[i] >= '0' && sw_num_char[i] <= '9')
                {
                    sw_decimal += (sw_num_char[i] - 48) * base;
                    base *= 16;
                }
                else if (sw_num_char[i] >= 'A' && sw_num_char[i] <= 'F')
                {
                    sw_decimal += (sw_num_char[i] - 55) * base;
                    base *= 16;
                }
                else if (sw_num_char[i] >= 'a' && sw_num_char[i] <= 'f')
                {
                    sw_decimal += (sw_num_char[i] - 87) * base;
                    base *= 16;
                }
            }
        }


        if (key == '1') {

            write(fd4, "run", 3);
   
            setting = 0;
        }
        else if (key == '2') {

            if (sw_decimal > 99) {
                sw_decimal = 99;
            }
            char sw_decimal_char[2];
            sprintf(sw_decimal_char, "%.2d", sw_decimal);
    

            start[6] = sw_decimal_char[0];
            start[7] = sw_decimal_char[1];
            write(fd4, &start, strlen(start));
            printf("%c%c%c%c%c%c%c%c \n", start[0], start[1], start[2], start[3], start[4], start[5], start[6], start[7]);

        }
        else if (key == '4') {

            if (sw_decimal > 59) {
                sw_decimal = 59;
            }
            char sw_decimal_char[2];
            sprintf(sw_decimal_char, "%.2d", sw_decimal);
         
            start[3] = sw_decimal_char[0];
            start[4] = sw_decimal_char[1];
        
            write(fd4, &start, strlen(start));
            printf("%c%c%c%c%c%c%c%c \n", start[0], start[1], start[2], start[3], start[4], start[5], start[6], start[7]);
        }
        else if (key == '8') {

            if (sw_decimal > 59) {
                sw_decimal = 59;
            }
            char sw_decimal_char[2];
            sprintf(sw_decimal_char, "%.2d", sw_decimal);
     

            start[0] = sw_decimal_char[0];
            start[1] = sw_decimal_char[1];
            write(fd4, &start, strlen(start));
            printf("%c%c%c%c%c%c%c%c \n", start[0], start[1], start[2], start[3], start[4], start[5], start[6], start[7]);
        }




    }
    char clean[4] = "0000";
    write(fd3, &clean, strlen(clean));


    int printquestion = 1;
    char timeleft[9] = "000000000";
    read(fd4, &timeleft, 9);
  
    int score = 0;
    int question_count = 0;
    int difficultly = 10;
    double total_used_time = 0;


    while (running) {
        
        srand(time(NULL));
        int number1 = rand() % difficultly;
        int number2 = rand() % difficultly;

        int answer = number1 + number2;

        int entered = -1;

       

        printf("%d + %d = ", number1, number2);
        

       

        while (running) {
            scanf("%d", &entered);
            printf("\033[1A");
            printf("\033[10C");
            printf("        %c%c%c%c%c%c%c%c \n", timeleft[0], timeleft[1], timeleft[2], timeleft[3], timeleft[4], timeleft[5], timeleft[6], timeleft[7]);
            read(fd4, &timeleft, 9);
            if (((timeleft[0] == '0') && (timeleft[1] == '0') && (timeleft[3] == '0') && (timeleft[4] == '0') && (timeleft[6] == '0') && (timeleft[7] == '0'))) {
                double average_time = total_used_time / question_count;
                printf("Time expired! You answered %d questions, in an average of %.2f seconds. \n", question_count, average_time);
                running = 0;
                break;
            }

            if (entered != answer && running==1) {
                printf("Try again: ");

            }
            else {
                score++;
                question_count++;
                int i = 0;
                int m, s, d;
                read(fd4, &timeleft, 9);
                i = sscanf(timeleft, "%02d:%02d:%02d", &m, &s, &d);
                total_used_time += m * 60 + s + d / 100;
                write(fd4, &start, strlen(start));
                break;
            }
   
        }

        if (score % 5 == 0)
            difficultly = difficultly * 10;


    }

    write(fd4, "nodisp", 6);
    close(fd1);
    close(fd2);
    close(fd3);
    close(fd4);
    return 0;
}
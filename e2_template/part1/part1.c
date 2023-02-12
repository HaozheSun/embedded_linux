#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include "address_map_arm.h"
#include "physical.h"


/**  your part 1 user code here  **/
int chartohex(char c){
	switch(c){
		case 'I':
			return 0x04;
		case 'n':
			return 0x54;
		case 't':
			return 0x78;
		case 'e':
			return 0x79;
		case 'l':
			return 0x38;
		case 'S':
			return 0x6D;
		case 'o':
			return 0x5C;
		case 'C':
			return 0x39;
		case 'F':
			return 0x71;
		case 'P':
			return 0x73;
		case 'G':
			return 0x7D;
		case 'A':
			return 0x77;
		default:
			return 0x00;
	}

}
int running=1;
void intHandler(int dummy){
	running=0;
}
int main(){
	char msg[]="Intel SoC FPGA Intel";
	int fd=-1;
	struct timespec ts;
	ts.tv_sec=0;
	ts.tv_nsec=500000000;
	void *LW_virtual;
	volatile int *KEY_ptr;
	volatile int *HEX3_HEX0_ptr;
	volatile int *HEX5_HEX4_ptr;
	fd = open_physical (fd);
	LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	KEY_ptr = (int *)(LW_virtual + KEY_BASE);
	HEX3_HEX0_ptr = (int *)(LW_virtual + HEX3_HEX0_BASE);
	HEX5_HEX4_ptr = (int *)(LW_virtual + HEX5_HEX4_BASE);
	*(KEY_ptr+3) = 0xF;
	int i=0;
	int keypressed=0;
	printf ("\e[2J");
	fflush (stdout);
	printf ("\e[01;01H");
    fflush (stdout);
	printf(" ------ ");
    fflush (stdout);
	printf ("\e[02;01H");
    fflush (stdout);
	printf("|      |");
    fflush (stdout);
	printf ("\e[03;01H");
    fflush (stdout);
	printf(" ------ ");
    fflush (stdout);
	while(running){
		signal(SIGINT, intHandler);
		printf ("\e[02;02H");
	    fflush (stdout);
		printf("%.6s", &msg[i]);
	    fflush (stdout);
		*HEX3_HEX0_ptr = chartohex(msg[i+2])<<24 | chartohex(msg[i+3])<<16 | chartohex(msg[i+4])<<8 | chartohex(msg[i+5]);
		*HEX5_HEX4_ptr = chartohex(msg[i])<<8 | chartohex(msg[i+1]);
		nanosleep(&ts, NULL);
		if (i == 14){
			i=0;
		}
		else{
			i++;
		}
		if (*(KEY_ptr+3) != 0x0){
			if (!keypressed){
				keypressed=1;
				*(KEY_ptr+3) = 0xF;
			}
		}
		while(keypressed){
			if (*(KEY_ptr+3) != 0x0){
				keypressed=0;
				*(KEY_ptr+3) = 0xF;
			}
		}

			
	}
	printf ("\e[2J");
	fflush (stdout);
	unmap_physical(LW_virtual, LW_BRIDGE_SPAN);
	close_physical (fd);
	return 0;
}

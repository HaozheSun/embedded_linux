#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <physical.h>
#include <linux/input.h>
#include <pthread.h>
#include "address_map_arm.h"
#include "video.h"
#include "defines.h"

/**  your part 4 user code here  **/
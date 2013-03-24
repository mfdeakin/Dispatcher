
#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#include "myipc.h"

#define MAXREQUESTS 5

const char *shmfname = "dispatch.inf";

struct request {
	int pid;
	int ticks;
};

struct dispatchbuffer {
	bool done;
	int clock;
	struct request requests[MAXREQUESTS];
	int rqsem;
	int rqemptysem;
	int rqfullsem;
	int cpuhasrqsem;
	int cpureadysem;
	int cpunextrq[];
};

#endif

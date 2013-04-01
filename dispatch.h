
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
	int sem;
};

struct cpu {
	int gosem;
	struct request rq;
};

struct dispatchbuffer {
	bool done;
	int shmid;
	/* clock doesn't need to be synchronized,
	 * as it will always be in a valid state */
	int clock;
	int cpucount;
	int jobbb, cpubb;
	struct cpu workers[];
};

#endif

/* Dispatcher
 * By Michael Deakin
 * /home/deakin_mf/csc460/hw/Dispatcher
 * This uses a shared library. Run as follows:
 * LD_LIBRARY_PATH="." ./startup [timeslice (sec)] [number of cpus]
 * and
 * LD_LIBRARY_PATH="." ./user [number of jobs]
 */


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
	int sem, sindex;
};

struct cpu {
	int gosem;
	bool hadjob;
	struct request rq;
};

struct dispatchbuffer {
	bool done;
	int shmid;
	/* clock doesn't need to be synchronized,
	 * as it will always be in a valid state,
	 * and there is only one writer, so no lost updates
	 */
	int clock;
	int timeslice;
	int cpucount;
	int jobbb, cpubb;
	struct cpu workers[];
};

#endif

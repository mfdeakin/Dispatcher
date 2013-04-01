
#include "dispatch.h"
#include "myipc.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>

void *getdispatcher(void);

int main(int argc, char **argv)
{
	if(argc < 2) {
		printf("Needs an argument N\n");
		return 1;
	}
	struct dispatchbuffer *disp = 
		(struct dispatchbuffer *)getdispatcher();
	if(!disp) {
		printf("Could not open shared memory!\n");
		return 2;
	}
	int n;
	sscanf(argv[1], "%d", &n);
	if(n == 0) {
		/* Shutdown */
		disp->done = true;
	}
	else {
		srand(time(NULL));
		struct request *rq = malloc(sizeof(struct request[n]));
		for(int i = 0; i < n; i++) {
			rq[i].pid = getpid();
			rq[i].ticks = (rand() % 8) + 2;
			rq[i].sem = getsem(1, 0);
			printf("Adding job with %d time to the bounded buffer %d\n",
						 rq[i].ticks, disp->jobbb);
			bbProduce(disp->jobbb, &rq[i]);
		}
		for(int i = 0; i < n; i++) {
			p(rq[i].sem, 0);
			semctl(rq[i].sem, 0, IPC_RMID);
		}
	}
}

void *getdispatcher(void)
{
	FILE *file = fopen(shmfname, "r");
	int shmid;
	if(file) {
		fseek(file, 0, SEEK_SET);
		fread(&shmid, sizeof(shmid), 1, file);
		return shmat(shmid, NULL, 0);
	}
	return NULL;
}

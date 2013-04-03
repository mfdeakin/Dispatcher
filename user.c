
#include "dispatch.h"
#include "myipc.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>

void *getdispatcher(void);
void simulate(struct dispatchbuffer *disp, int jobs);

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
		simulate(disp, n);
	}
}

void simulate(struct dispatchbuffer *disp, int jobs)
{
	srand(time(NULL));
	struct request *rq = malloc(sizeof(struct request[jobs]));
	int sem = getsem(jobs, 0);
	int totrt = 0;
	for(int i = 0; i < jobs; i++) {
		rq[i].pid = getpid();
		rq[i].ticks = (rand() % 5) + 1;
		rq[i].sem = sem;
		rq[i].sindex = i;
		int thinktime = (rand() % 9) + 2;
		printf("\t%d is thinking for %d seconds\n",
					 rq[i].pid, thinktime);
		sleep(thinktime);
		int starttime = disp->clock;
		printf("\t%d requests %d\n", rq[i].pid, rq[i].ticks);
		bbProduce(disp->jobbb, &rq[i]);
		p(rq[i].sem, i);
		int rtime = disp->clock - starttime;
		printf("\t%d %d finished in %d seconds\n",
					 rq[i].pid, i, rtime);
		totrt += rtime;
	}
	printf("\t%d is logging off with average response time=%f s\n",
				 rq[0].pid, ((float)totrt) / jobs);
	free(rq);
	shmdt(disp);
	semctl(sem, 0, IPC_RMID);
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

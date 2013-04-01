
#include "dispatch.h"

#include <sys/sem.h>
#include <sys/shm.h>

const int DEFTIMESLICE = 1;
const int DEFCPUS = 1;
const int DEFBUFSIZE = 5;

struct dispatchbuffer *init(int cpus);
void cleanup(struct dispatchbuffer *disp);
void runClock(struct dispatchbuffer *disp);
void runCPU(struct dispatchbuffer *disp, int cpu);
void dispatch(struct dispatchbuffer *disp);

int main(int argc, char **argv)
{
	int timeslice = DEFTIMESLICE;
	int cpus = DEFCPUS;
	if(argc > 1) {
		int check = sscanf(argv[1], "%d", &timeslice);
		if(check == 0) {
			printf("Unknown parameter\n");
			return 0;
		}
		if(argc > 2) {
			check = sscanf(argv[2], "%d", &cpus);
			if(check == 0) {
				printf("Unknown parameter\n");
				return 0;
			}
		}
	}
	struct dispatchbuffer *disp = init(cpus);
	/* Create the clock */
	if(fork()) {
		runClock(disp);
		/* The clock is responsible for cleanup, as the others will probably be
		 * waiting for a semaphore to be unlocked that never will.
		 */
		cleanup(disp);
	}
	/* Create the CPU */
	else if(fork()) {
		int id;
		for(id = 0; id < (cpus - 1) && fork(); id++);
		runCPU(disp, id);
	}
	else {
		/* Create the dispatcher */
		dispatch(disp);
	}
	return 0;
}

void dispatch(struct dispatchbuffer *disp)
{
	printf("Let the Simulation Begin!!\n");
	/* Get the jobs from the bounded buffer for jobs,
	 * Get a CPU from the bounded buffer for cpus,
	 * Set that CPU to execute that job
	 */
	while(!disp->done) {
		int *cpu = (int *)bbConsume(disp->cpubb);
		if(!cpu)
			return;
		struct request *rq = (struct request *)bbConsume(disp->jobbb);
		if(!rq)
			return;
		disp->workers[*cpu].rq = *rq;
		v(disp->workers[*cpu].gosem, 0);
	}
}

void runCPU(struct dispatchbuffer *disp, int cpu)
{
	while(!disp->done) {
		bbProduce(disp->cpubb, &cpu);
		if(p(disp->workers[cpu].gosem, 0))
			return;
		struct request rq = disp->workers[cpu].rq;
		v(disp->workers[cpu].rq.sem, 0);
		printf("\t\tCPU %d receives request for %d seconds from %d\n",
					 cpu, rq.ticks, rq.pid);
		/* Let the user process finish */
		sleep(rq.ticks);
		printf("\t\t\t\tCPU %d finished request for %d\n", cpu, rq.pid);
	}
}

void runClock(struct dispatchbuffer *disp)
{
	while(!disp->done) {
		sleep(1);
		disp->clock++;
		printf("%d\n", disp->clock);
	}
}

struct dispatchbuffer *init(int cpus)
{
	int shmid;
	struct dispatchbuffer *buffer =
		getshm(sizeof(struct dispatchbuffer) + sizeof(struct cpu[cpus]), &shmid);
	FILE *shmfile = fopen(shmfname, "w");
	fseek(shmfile, 0, SEEK_SET);
	fwrite(&shmid, sizeof(shmid), 1, shmfile);
	fclose(shmfile);
	buffer->shmid = shmid;
	buffer->clock = 0;
	buffer->done = false;
	buffer->jobbb = bbCreate(DEFBUFSIZE, sizeof(struct request));
	buffer->cpubb = bbCreate(cpus, sizeof(int));
	buffer->cpucount = cpus;
	for(int i = 0; i < cpus; i++) {
		buffer->workers[i].gosem = getsem(1, 0);
	}
	return buffer;
}

void cleanup(struct dispatchbuffer *disp)
{
	unlink(shmfname);
	for(int i = 0; i < disp->cpucount; i++)
		semctl(disp->workers[i].gosem, 0, IPC_RMID);
	bbFree(disp->jobbb);
	bbFree(disp->cpubb);
	shmctl(disp->shmid, IPC_RMID, NULL);
	shmdt(disp);
}

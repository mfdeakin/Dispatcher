
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
		p(disp->rcsem, 0);
		disp->runcnt++;
		v(disp->rcsem, 0);
		runClock(disp);
	}
	/* Create the CPU */
	else if(fork()) {
		int id;
		for(id = 0; id < cpus && fork(); id++);
		p(disp->rcsem, 0);
		disp->runcnt++;
		v(disp->rcsem, 0);
		runCPU(disp, id);
	}
	else {
		printf("Let the Simulation Begin!!\n");
		p(disp->rcsem, 0);
		disp->runcnt++;
		v(disp->rcsem, 0);
		dispatch(disp);
	}
	p(disp->rcsem, 0);
	disp->runcnt--;
	if(disp->runcnt == 0)
		cleanup(disp);
	else
		v(disp->rcsem, 0);
	return 0;
}

void dispatch(struct dispatchbuffer *disp)
{
	/* Get the jobs from the bounded buffer for jobs,
	 * Get a CPU from the bounded buffer for cpus,
	 * Set that CPU to execute that job
	 */
	while(!disp->done) {
		struct request *rq = (struct request *)bbConsume(disp->jobs);
		int *cpu = (int *)bbConsume(disp->cpus);
		disp->workers[*cpu].rq = *rq;
		v(disp->workers[*cpu].gosem, 0);
	}
}

void runCPU(struct dispatchbuffer *disp, int cpu)
{
	while(!disp->done) {
		bbProduce(disp->cpus, &cpu);
		p(disp->workers[cpu].gosem, 0);
		printf("\t\tCPU receives request for %d seconds from %d\n",
					 disp->workers[cpu].rq.ticks, disp->workers[cpu].rq.pid);
		sleep(disp->workers[cpu].rq.ticks);
		v(disp->workers[cpu].rq.sem);
	}
}

void runClock(struct dispatchbuffer *disp)
{
	while(disp->done) {
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
	buffer->jobs = bbCreate(DEFBUFSIZE, sizeof(struct request));
	buffer->cpus = bbCreate(cpus, sizeof(int));
	buffer->runcnt = 0;
	buffer->cpucount = cpus;
	buffer->rcsem = getsem(1, 1);
	for(int i = 0; i < cpus; i++)
		buffer->workers[i].gosem = getsem(1, 1);
	return buffer;
}

void cleanup(struct dispatchbuffer *disp)
{
	unlink(shmfname);
	semctl(disp->rcsem, 0, IPC_RMID);
	for(int i = 0; i < disp->cpucount; i++)
		semctl(disp->workers[i].gosem, 0, IPC_RMID);
	shmctl(disp->shmid, IPC_RMID, NULL);
	shmdt(disp);
}

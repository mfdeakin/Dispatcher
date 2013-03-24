
#include "dispatch.h"

const int DEFTIMESLICE = 1;
const int DEFCPUS = 1;

struct dispatchbuffer *init(int cpus);
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
	}
	/* Create the CPU */
	else if(fork()) {
		int id;
		for(id = 0; id < cpus && fork(); id++);
		runCPU(disp, id);
	}
	else {
		printf("Let the Simulation Begin!!\n");
		dispatch(disp);
	}
	return 0;
}

void dispatch(struct dispatchbuffer *disp)
{
	
}

void runCPU(struct dispatchbuffer *disp, int cpu)
{
	for(; disp->done;) {
		p(disp->cpuhasrqsem, cpu);
		struct request rq = disp->cpunextrq[cpu];
		printf("\t\tCPU receives request for %d from %d\n", rq.ticks, rq.pid);
		sleep(1);
	}
}

void runClock(struct dispatchbuffer *disp)
{
	for(; disp->done;) {
		sleep(1);
		disp->clock++;
		printf("%d\n", disp->clock);
	}
}

struct dispatchbuffer *init(int cpus)
{
	int shmid;
	struct dispatchbuffer *buffer =
		getshm(sizeof(struct dispatchbuffer) + sizeof(int[cpus]),
					 &shmid);
	FILE *shmfile = fopen(shmfname, "w");
	fseek(shmfile, 0, SEEK_SET);
	fwrite(&shmid, sizeof(shmid), 1, shmfile);
	fclose(shmfile);
	buffer->clock = 0;
	buffer->rqemptysem = getsem(1, MAXREQUESTS);
	buffer->rqfullsem = getsem(1, 0);
	buffer->cpuhasrqsem = getsem(cpus, 0);
	buffer->rqsem = getsem(1, 0);
	return buffer;
}

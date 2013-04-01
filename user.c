
#include "dispatch.h"
#include "myipc.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>

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
		
	}
	else {
		srand(time(NULL));
		for(int i = 0; i < n; i++) {
			rand();
		}
	}
}

void *getdispatcher(void)
{
	FILE *file = fopen(shmfname, "r");
	int shmid;
	fscanf(file, "%d", &shmid);
	fclose(file);
	return shmat(shmid, NULL, 0);
}


#include "myipc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>

void *getshm(int size, int *id)
{
	int shmid = shmget(IPC_PRIVATE, size, 0600);
	if(!shmid)
		return NULL;
	void *memory = shmat(shmid, NULL, 0);
	memset(memory, 0, size);
	if(id)
		*id = shmid;
	return memory;
}

/* Locks the critical section */
void p(int semaphore, int num)
{
	struct sembuf operations;
	operations.sem_num = num;
	operations.sem_op = -1;
	operations.sem_flg = 0;
	if(semop(semaphore, &operations, 1) == -1) {
		printf("P Error\n");
	}
}

/* Unlocks the critical section */
void v(int semaphore, int num)
{
	struct sembuf operations;
	operations.sem_num = num;
	operations.sem_op = 1;
	operations.sem_flg = 0;
	if(semop(semaphore, &operations, 1) == -1) {
		printf("V Error\n");
	}
}

/* Gets count semaphores.
 * The semaphore is initialized with the initial value
 */
int getsem(int count, int initial)
{
	int semid = semget(IPC_PRIVATE, count, 0777);
	if(semid != -1) {
		for(int i = 0; i < count; i++)
			semctl(semid, i, SETVAL, initial);
	}
	return semid;
}

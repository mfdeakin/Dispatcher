
#include "myipc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>

struct boundedbuf {
	/* 0 is the heap lock
	 * 1 is the empty lock
	 * 2 is the full lock
	 */
	int semaphores;
	int shmid;
	size_t elsize;
	int count, max;
	/* Can't make it void array, which is more appropriate here */
	char buffer[];
};

int bbCreate(unsigned size, size_t elsize)
{
	int shmid;
	struct boundedbuf *bb = getshm(sizeof(struct boundedbuf) + elsize * size,
																 &shmid);
	bb->shmid = shmid;
	bb->count = 0;
	bb->max = size;
	bb->elsize = elsize;
	bb->semaphores = getsem(3, 0);
	/* Semaphore 0 is the buffer block */
	semctl(bb->semaphores, 0, SETVAL, 1);
	/* Semaphore 1 is the empty block */
	semctl(bb->semaphores, 1, SETVAL, size);
	/* Semaphore 2 is the full block */
	semctl(bb->semaphores, 2, SETVAL, 0);
	shmdt(bb);
	return shmid;
}

void bbFree(int bbmem)
{
	struct boundedbuf *bb = shmat(bbmem, NULL, 0);
	semctl(bb->semaphores, 0, IPC_RMID);
	shmdt(bb);
	shmctl(bbmem, IPC_RMID, 0);
}

void *bbConsume(int bbmem)
{
	struct boundedbuf *bb = shmat(bbmem, NULL, 0);
	/* Make certain there is an item in the bounded buffer */
	if(p(bb->semaphores, 2)) {
		return NULL;
	}
	if(p(bb->semaphores, 0)) {
		v(bb->semaphores, 2);
		return NULL;
	}
	/* Remove the item from the bounded buffer after locking it */
	void *el = malloc(bb->elsize);
	memcpy(el, bb->buffer, bb->elsize);
	for(int i = 0; i < bb->count - 1; i++)
		memcpy(bb->buffer + i * bb->elsize, bb->buffer + (i + 1) * bb->elsize,
					 bb->elsize);
	bb->count--;
	v(bb->semaphores, 0);
	/* Let any producers know there's room in the bounded buffer */
	v(bb->semaphores, 1);
	shmdt(bb);
	return el;
}

void bbProduce(int bbmem, void *consumable)
{
	struct boundedbuf *bb = shmat(bbmem, NULL, 0);
	/* Make certain there is room in the bounded buffer */
	if(p(bb->semaphores, 1)) {
		return;
	}
	/* Add the item to the bounded buffer after locking it */
	if(p(bb->semaphores, 0)) {
		v(bb->semaphores, 1);
		return;
	}
	memcpy(bb->buffer + bb->elsize * bb->count, consumable, bb->elsize);
	bb->count++;
	v(bb->semaphores, 0);
	/* Let any consumers know there's an item in the bounded buffer */
	v(bb->semaphores, 2);
	shmdt(bb);
}

void *getshm(int size, int *id)
{
	int shmid = shmget(IPC_PRIVATE, size, 0666);
	if(!shmid)
		return NULL;
	void *memory = shmat(shmid, NULL, 0);
	memset(memory, 0, size);
	/* Let the program have the id if it wants,
	 * otherwise mark the memory for deletion so
	 * when the program is done with it, it isn't
	 * left lying around
	 */
	if(id)
		*id = shmid;
	else
		shmctl(shmid, IPC_RMID, NULL);
	return memory;
}

/* Locks the critical section */
bool p(int semaphore, int num)
{
	struct sembuf operations;
	operations.sem_num = num;
	operations.sem_op = -1;
	operations.sem_flg = 0;
	if(semop(semaphore, &operations, 1) == -1)
		return true;
	return false;
}

/* Unlocks the critical section */
void v(int semaphore, int num)
{
	struct sembuf operations;
	operations.sem_num = num;
	operations.sem_op = 1;
	operations.sem_flg = 0;
	if(semop(semaphore, &operations, 1) == -1) {
		printf("V Error: %d  %d\n", semaphore, num);
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

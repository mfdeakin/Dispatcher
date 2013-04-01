
#include "myipc.h"
#include "heap.h"

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
	unsigned idnum;
	int hpbuffer;
};

struct bbelem {
	unsigned id;
	/* Can't make it void array, which is more appropriate here */
	char bufitem[];
};

int compare(const void *lhs, const void *rhs)
{
	struct bbelem *blhs = (struct bbelem *)lhs,
		*brhs = (struct bbelem *)rhs;
	return blhs->id - brhs->id;
}

struct boundedbuf *bbCreate(unsigned size, size_t elsize)
{
	int shmid;
	struct boundedbuf *bb = getshm(sizeof(struct boundedbuf), &shmid);
	bb->shmid = shmid;
	bb->idnum = 0;
	bb->elsize += sizeof(struct bbelem);
	bb->hpbuffer = hpCreate(&compare, bb->elsize);
	bb->semaphores = getsem(3, 0);
	/* Semaphore 0 is the buffer block */
	semctl(bb->semaphores, 0, SETVAL, 1);
	/* Semaphore 1 is the empty block */
	semctl(bb->semaphores, 1, SETVAL, size);
	/* Semaphore 2 is the full block */
	semctl(bb->semaphores, 2, SETVAL, 0);
	return bb;
}

void bbFree(struct boundedbuf *bb)
{
	hpFree(bb->hpbuffer);
	semctl(bb->semaphores, 0, IPC_RMID);	
	shmdt(bb);
}

void *bbConsume(struct boundedbuf *bb)
{
	/* Make certain there is an item in the bounded buffer */
	p(bb->semaphores, 2);
	/* Remove the item from the bounded buffer after locking it */
	p(bb->semaphores, 0);
	struct bbelem *el = hpTop(bb->hpbuffer);
	v(bb->semaphores, 0);
	/* Let any producers know there's room in the bounded buffer */
	v(bb->semaphores, 1);
	return el->bufitem;
}

void bbProduce(struct boundedbuf *bb, void *consumable)
{
	/* Make certain there is room in the bounded buffer */
	p(bb->semaphores, 1);
	/* Add the item to the bounded buffer after locking it */
	struct bbelem *el = (struct bbelem *)malloc(bb->elsize);
	memcpy(el->bufitem, consumable, bb->elsize - sizeof(struct bbelem));
	p(bb->semaphores, 0);
	bb->idnum++;
	el->id = bb->idnum;
	hpAdd(bb->hpbuffer, el);
	v(bb->semaphores, 0);
	/* Let any consumers know there's an item in the bounded buffer */
	v(bb->semaphores, 2);
}

void *getshm(int size, int *id)
{
	int shmid = shmget(IPC_PRIVATE, size, 0600);
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

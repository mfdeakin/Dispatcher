
#include "heap.h"
#include "myipc.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/shm.h>

struct heap
{
	void *heap;
	int shmid;
	int (*compare)(const void *, const void *);
	unsigned count, max;
	size_t elsize;
};

void hpAllocMem(struct heap *hp);
void *hpElemAddr(struct heap *hp, unsigned elem);
void hpSetElem(struct heap *hp, unsigned elem, void *source);

int hpCreate(int (*compare)(const void *, const void *),
											size_t elsize)
{
	int shmid;
	struct heap *hp = getshm(sizeof(struct heap), &shmid);
	hp->shmid = shmid;
	hp->compare = compare;
	hp->elsize = elsize;
	hp->count = 0;
	hp->max = 1;
	hp->heap = NULL;
	shmdt(hp);
	return shmid;
}

void hpFree(int heap)
{
	struct heap *hp = shmat(heap, NULL, 0);
	shmdt(hp->heap);
	shmctl(hp->shmid, IPC_RMID, NULL);
	shmdt(hp);
}

void hpAdd(int heap, void *data)
{
	struct heap *hp = shmat(heap, NULL, 0);
	hp->count++;
	if(hp->count >= hp->max) {
		hp->max *= 2;
		hpAllocMem(hp);
	}
	if(hp->count == 1) {
		hpSetElem(hp, 0, data);
	}
	else {
		unsigned pos = hp->count - 1;
		bool cont = true;
		while(cont) {
			if(pos == 0) {
				hpSetElem(hp, pos, data);
				break;
			}
			if(hp->compare(data, hpElemAddr(hp, pos / 2)) < 0) {
				hpSetElem(hp, pos, data);
				cont = false;
			}
			else {
				pos /= 2;
				hpSetElem(hp, pos, hpElemAddr(hp, pos));
			}
		}
	}
	shmdt(hp);
}

void *hpPeek(int heap)
{
	struct heap *hp = shmat(heap, NULL, 0);
	if(hp->count > 0) {
		void *buf = malloc(hp->elsize);
		memcpy(buf, hp->heap, hp->elsize);
		shmdt(hp);
		return buf;
	}
	shmdt(hp);
	return NULL;
}

void *hpTop(int heap)
{
	struct heap *hp = shmat(heap, NULL, 0);
	if(hp->count == 0) {
		shmdt(hp);
		return NULL;
	}
	hp->count--;
	void *top = malloc(hp->elsize);
	memcpy(top, hp->heap, hp->elsize);
	hpSetElem(hp, 0, hpElemAddr(hp, hp->count));
	memset(hpElemAddr(hp, hp->count), 0, hp->elsize);
	unsigned pos = 1;
	void *buffer = malloc(hp->elsize);
	while(pos * 2 < hp->count &&
				(hp->compare(hpElemAddr(hp, pos - 1), hpElemAddr(hp, pos * 2 - 1)) ||
				 hp->compare(hpElemAddr(hp, pos - 1), hpElemAddr(hp, pos * 2)))) {
		memcpy(buffer, hpElemAddr(hp, pos - 1), hp->elsize);
		if(hp->compare(hpElemAddr(hp, pos * 2 - 1), hpElemAddr(hp, pos * 2))) {
			hpSetElem(hp, pos - 1, hpElemAddr(hp, pos * 2 - 1));
			hpSetElem(hp, pos * 2 - 1, buffer);
			pos = pos * 2 - 1;
		}
		else {
			hpSetElem(hp, pos - 1, hpElemAddr(hp, pos * 2));
			hpSetElem(hp, pos * 2 - 1, buffer);
			pos = pos * 2;
		}
	}
	free(buffer);
	shmdt(hp);
	return top;
}

unsigned hpSize(int heap)
{
	struct heap *hp = shmat(heap, NULL, 0);
	unsigned count = hp->count;
	shmdt(hp);
	return count;
}

void hpAllocMem(struct heap *hp)
{
	void *newmem = getshm(hp->max, NULL);
	if(hp->heap) {
		memcpy(newmem, hp->heap, hp->elsize * hp->count);
		shmdt(hp->heap);
	}
	hp->heap = newmem;
}

void *hpElemAddr(struct heap *hp, unsigned elem)
{
	return hp->heap + elem * hp->elsize;
}

void hpSetElem(struct heap *hp, unsigned elem, void *source)
{
	memcpy(hpElemAddr(hp, elem), source, hp->elsize);
}

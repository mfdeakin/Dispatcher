
#ifndef _LIBIPC_H_
#define _LIBIPC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Initializes a shared memory segment of size size
 * If shmid is not NULL, it will end with the shared memory id
 * If shmid is NULL, the segment is marked for deletion after
 * the process has detached from it.
 */
void *getshm(int size, int *shmid);
/* Attempts to allocate an array of count semaphores
 * with initial value initial
 */
int getsem(int count, int initial);
/* Locks the critical section */
void p(int semaphore, int num);
/* Unlocks the critical section */
void v(int semaphore, int num);

/* A solution to the bounded buffer problem */
struct boundedbuf;

struct boundedbuf *bbCreate(unsigned bbsize, size_t elsize);
void bbFree(struct boundedbuf *);
void *bbConsume(struct boundedbuf *);
void bbProduce(struct boundedbuf *, void *consumable);


#ifdef __cplusplus
};
#endif

#endif

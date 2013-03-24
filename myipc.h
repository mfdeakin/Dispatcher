
#ifndef _LIBIPC_H_
#define _LIBIPC_H_

#ifdef __cplusplus
extern "C" {
#endif

void *getshm(int size, int *shmid);

int getsem(int count, int initial);
void p(int semaphore, int num);
void v(int semaphore, int num);

#ifdef __cplusplus
};
#endif

#endif


#ifndef __HEAP_H
#define __HEAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	/* I made the foolish mistake of adapting a heap I wrote 
	 * to use with shared memory. It uses way too many sys calls */

typedef struct heap heap;

int hpCreate(int (*compare)(const void *lhs, const void *rhs),
						 size_t elsize);
void hpFree(int hp);
void hpAdd(int hp, void *data);
void *hpPeek(int hp);
void *hpTop(int hp);
unsigned hpSize(int hp);

#ifdef __cplusplus
}
#endif

#endif

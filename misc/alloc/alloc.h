#ifndef _ALLOC_H_
#define _ALLOC_H_

#include <string.h>

/* Add a block of memory to the allocation pool. The allocator initially
 * starts with an empty pool.
 */
int add_block(void *ptr, size_t size);

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);

#endif /* _ALLOC_H_ */

#ifndef _MEMORY_H
#define _MEMORY_H

#include <time.h>
#include <pthread.h>

#define MEMO_LEN 64
#define MAX_ALLOCATION 4096

typedef struct allocation{
	void*	pointer;
	time_t	time_alloc;
	int	size;
	char	memo[MEMO_LEN];
}allocation;

typedef struct mem_detail{
	int item_count;
	allocation* items[MAX_ALLOCATION];
	pthread_mutex_t	mutex_mem;
}mem_detail;


#define NEW( p, size ) { p = malloc(size); memset( p, 0, size ); }
#define DEL( p ) { free((void*)p); p = NULL; }

//
//#define NEW( p, size ) memory_new_detail( (void**)&p, size, __FILE__, (char*)__func__, __LINE__, #p );
//#define MNEW( p, size, memo ) memory_new( (void**)&p, size, memo );
//#define DEL( p ) {memory_delete( (void*)p ); p = NULL;}


void memory_init();
void memory_new( void** p, int size, char* memo );
void memory_new_detail( void** p, int size, char* file, char* function, int line, char* name );
void memory_delete( void* p );
void memory_end();
void memory_print();

#endif


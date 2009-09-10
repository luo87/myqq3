#ifndef _LIST_H
#define _LIST_H

#include <pthread.h>

typedef struct list{
	pthread_mutex_t	mutex;
	int		size;
	int		count;
	void**		items;
}list;

typedef int (*comp_func)(const void *, const void *);
typedef int (*search_func)(const void *, const void *);

int list_create( list* l, int size );
int list_append( list* l, void* data );
int list_remove( list* l, void* data );
void* list_search( list* l, void*, search_func search );
void list_sort( list* l, comp_func comp );
void list_cleanup( list* l );

#endif

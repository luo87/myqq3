#ifndef _LOOP_H
#define _LOOP_H

#include <pthread.h>

typedef int (*loop_search_func)(const void *, const void *);
typedef void (*loop_delete_func)(const void *);

typedef struct loop{
	pthread_mutex_t	mutex;
	int		size;
	int		head;
	int		tail;
	char		str[5];
	void**		items;
	loop_delete_func	del_func;	//溢出时，是否删除
}loop;

int loop_create( loop* l, int size, loop_delete_func del );
int loop_append( loop* l, void* data );
int loop_r_append( loop* l, void* data );
void* loop_pop_from_tail( loop* l );
void* loop_pop_from_head( loop* l );
int loop_push_to_head( loop* l, void* data );
int loop_push_to_tail( loop* l, void* data );
void* loop_search( loop* l, void*, loop_search_func search );
void loop_cleanup( loop* l );
int loop_is_empty( loop* l );
void loop_remove( loop* l, void* data );

#endif

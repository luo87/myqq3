/*
 *  loop.c
 *
 *  A Loop Queue
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *
 */

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "qqdef.h"
#include "debug.h"
#include "memory.h"
#include "loop.h"


int loop_create( loop* l, int size, loop_delete_func del )
{
	l->size = size;
	l->head = l->tail = 0;
	l->del_func = del;
	pthread_mutex_init( &l->mutex, NULL );
	l->items = malloc( l->size*sizeof(void*) );
	assert( l->items != NULL );
	return 0;
}

//加到尾
int loop_push_to_tail( loop* l, void* data )
{
	pthread_mutex_lock( &l->mutex );
	if( (l->tail+1)%l->size == l->head ){
	//	DBG("loop is full. size:%d", l->size);
		if( l->del_func )
			l->del_func( l->items[l->head] );
		l->head = (l->head+1)%l->size;
		
	}
	l->items[l->tail] = data;
	l->tail = (l->tail+1)%l->size;
	pthread_mutex_unlock( &l->mutex );
	return 0;
}

//加到头
int loop_push_to_head( loop* l, void* data )
{
	pthread_mutex_lock( &l->mutex );
	if( (l->size+l->head-1)%l->size == l->tail ){
		l->tail = (l->size+l->tail-1)%l->size;
		assert( l->tail >= 0 );
		if( l->del_func )
			l->del_func( l->items[l->tail] );
		
	}
	l->head = (l->size+l->head-1)%l->size;
	assert( l->head >= 0 );
	l->items[l->head] = data;
	pthread_mutex_unlock( &l->mutex );
	return 0;
}

void* loop_pop_from_head( loop* l )
{
	void* p = NULL;
	pthread_mutex_lock( &l->mutex );
	if( l->tail != l->head ){
		p = l->items[l->head];
		l->head = (l->head+1)%l->size;
		assert( l->head < l->size );
	}
	pthread_mutex_unlock( &l->mutex );
	return p;
}

void* loop_pop_from_tail( loop* l )
{
	void* p = NULL;
	pthread_mutex_lock( &l->mutex );
	if( l->tail != l->head ){
		l->tail = (l->size+l->tail-1)%l->size;
		assert( l->tail >= 0 );
		p = l->items[l->tail];
	}
	pthread_mutex_unlock( &l->mutex );
	return p;
}

void loop_remove( loop* l, void* data )
{
	int i;
	pthread_mutex_lock( &l->mutex );
	for( i=l->head; i!=l->tail; i=(i+1)%l->size )
	{
		if( l->items[i] == data ){
			l->tail = (l->size+l->tail-1)%l->size;
			assert( l->tail >= 0 );
			//从i开始，数据往前移动
			for( ; i!=l->tail; i=(i+1)%l->size )
				l->items[i] = l->items[(i+1)%l->size];
			break;
		}
	}
	pthread_mutex_unlock( &l->mutex );
}

void* loop_search( loop* l, void* v, loop_search_func search )
{
	int i;
	pthread_mutex_lock( &l->mutex );
	for( i=l->head; i!=l->tail; i=(i+1)%l->size )
	{
		if( search( l->items[i], v ) )
			break;
	}
	pthread_mutex_unlock( &l->mutex );
	if( i != l->tail )
		return l->items[i];
	return NULL;
}

void loop_cleanup( loop* l )
{
	int i;
	pthread_mutex_lock( &l->mutex );
	if( l->del_func )
		for( i=l->head; i!=l->tail; i=(i+1)%l->size )
			l->del_func( l->items[i] );
	free( l->items );
	pthread_mutex_unlock( &l->mutex );
	pthread_mutex_destroy( &l->mutex );
}

int loop_is_empty( loop* l )
{
	return( l->head == l->tail );
}


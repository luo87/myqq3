/*
 *  list.c
 *
 *  List
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
#include "list.h"


int list_create( list* l, int size )
{
	l->size = size;
	l->count = 0;
	pthread_mutex_init( &l->mutex, NULL );
	NEW( l->items, l->size*sizeof(void*) );
	assert( l->items );
	return 0;
}

int list_append( list* l, void* data )
{
	int i, ret = 0;
	pthread_mutex_lock( &l->mutex );
	if( l->count >= l->size ){
		DBG("list is full. count:%d", l->count);
		ret = -1;
	}else{
		i = l->count ++;
		l->items[i] = data;
	}
	pthread_mutex_unlock( &l->mutex );
	return ret;
}

int list_remove( list* l, void* data )
{
	int i;
	pthread_mutex_lock( &l->mutex );
	for( i=0; i<l->count; i++ ){
		if( l->items[i] == data ){
			l->count --;
			if( i != l->count )
				l->items[i] = l->items[l->count];
			break;
		}
	}
	pthread_mutex_unlock( &l->mutex );
	return 0;
}

void* list_search( list* l, void* v, search_func search )
{
	int i;
	pthread_mutex_lock( &l->mutex );
	for( i=0; i<l->count; i++ ){
		if( search( l->items[i], v ) )
			break;
	}
	pthread_mutex_unlock( &l->mutex );
	if( i < l->count )
		return l->items[i];
	return NULL;
}

void list_sort( list* l, comp_func comp )
{
	pthread_mutex_lock( &l->mutex );
	if( l->count == 0 )
		return;
	qsort( l->items, l->count, sizeof(void*), comp );
	pthread_mutex_unlock( &l->mutex );
}

void list_cleanup( list* l )
{
	int i;
	pthread_mutex_lock( &l->mutex );
	if( l->count > 0 ){
	//	DBG("list count = %d", l->count );
		for( i=0; i<l->count; i++ ){
			DEL( l->items[i] );
		}
	}
	DEL( l->items );
	pthread_mutex_unlock( &l->mutex );
	pthread_mutex_destroy( &l->mutex );
}



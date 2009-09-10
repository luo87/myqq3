/*
 *  memory.c
 *
 *  Memory Management
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-9 15:40:05 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  allocating/releasing memory.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "debug.h"
#include "memory.h"

mem_detail* g_md;

void memory_init()
{
	if( !g_md ){
		g_md = (mem_detail*)malloc(sizeof(mem_detail));
		memset( g_md, 0, sizeof(mem_detail) );
		pthread_mutex_init( &g_md->mutex_mem, NULL );
	}
}

void memory_new_detail( void** p, int size, char* file, char* function, int line, char* name )
{
	char temp[128];
	sprintf( temp, "[%s]%s(%d) %s", file, function, line, name );
	memory_new( p, size, temp );
}

void memory_new( void** p, int size, char* memo )
{
	if( !g_md )
		memory_init();
	*p = NULL;
	if( g_md->item_count >= MAX_ALLOCATION )
	{
		DBG("no more allocations."); return;
	}
	*p = malloc(size);
	memset( *p, 0, size );
	if( *p == NULL ){
		DBG("no enough memory.");
		exit(-1);
	}
	pthread_mutex_lock( &g_md->mutex_mem );
	int i = g_md->item_count ++;
	g_md->items[i] = (allocation*)malloc(sizeof(allocation));
	//zero
	g_md->items[i]->pointer = *p;
	g_md->items[i]->size = size;
	strncpy( g_md->items[i]->memo, memo, MEMO_LEN );
	g_md->items[i]->time_alloc = time( NULL );
	pthread_mutex_unlock( &g_md->mutex_mem );
	//ok
}

//a piece of very lazy code
void memory_delete( void* p )
{
	int i;
	if( p == NULL ) return;
	pthread_mutex_lock( &g_md->mutex_mem );
	for( i=0; i<g_md->item_count; i++ )
	{
		if( g_md->items[i]->pointer == p )
		{
			free( p );
			free( g_md->items[i] );
			g_md->item_count --;
			if( i!=g_md->item_count )
				g_md->items[i] = g_md->items[g_md->item_count];
			g_md->items[g_md->item_count] = NULL;
			pthread_mutex_unlock( &g_md->mutex_mem );
			return;
		}
	}
	DBG("not found pointer %x", p );
	pthread_mutex_unlock( &g_md->mutex_mem );
}

void memory_end(){
	if( !g_md ) return;
	DBG("g_md->item_count = %d", g_md->item_count );
	if( g_md->item_count>0 ){
		memory_print();
	}
	pthread_mutex_destroy( &g_md->mutex_mem );
}

void memory_print(){
	if( !g_md ) return;
	DBG("#memory info dumping (item_count: %d) ", g_md->item_count );
	int i;
	pthread_mutex_lock( &g_md->mutex_mem );
	for( i=0; i<g_md->item_count; i++ )
	{
		char timestr[10];
		struct tm* t = localtime( & g_md->items[i]->time_alloc);
		strftime( timestr, 10, "%X", t );
		DBG("[%d] 0x%x(%d)\t%s\t%s", i, g_md->items[i]->pointer, g_md->items[i]->size, 
			g_md->items[i]->memo, timestr );
	}
	pthread_mutex_unlock( &g_md->mutex_mem );
}

//test
/*
int main()
{
	memory_init();
	char* p, *str2;
	NEW( str2, 1024 );
	NEW( p, 256 );
	strcpy( str2, "hello");
	puts( str2 );
	DEL( p );
	memory_end();
	return 0;
}
*/

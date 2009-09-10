/*
 *  group.c
 *
 *  Group Operations
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *
 */

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "qqclient.h"
#include "memory.h"
#include "debug.h"
#include "protocol.h"
#include "list.h"
#include "group.h"

static int searcher( const void* p, const void* v )
{
	return ( ((qqgroup*)p)->number == (int)v );
}

qqgroup* group_get( struct qqclient* qq, uint number, int create )
{
	if( !number )
		return NULL;
	qqgroup* g;
	g = list_search( &qq->group_list, (void*)number, searcher );
	//if not found, g must be NULL
	if( !g && create ){
		NEW( g, sizeof( qqgroup ) );
		if( !g ){
			DBG("Fatal error: group not allocated"); 
			return g;
		}
		g->number = number;
		sprintf( g->name, "%u", number );
		if( list_append( &qq->group_list, (void*)g )<0 ){
			DEL( g );
			DBG("group list is full.");
		}
	}
	return g;
}

void group_remove( struct qqclient* qq, uint number )
{
	qqgroup* g;
	g = list_search( &qq->group_list, (void*)number, searcher );
	if( g ){
		list_remove( &qq->group_list, g );
	}
}

void group_update_list( struct qqclient* qq )
{
//	prot_group_download_list( qq, 0 );  不知道这个09还能不能用。
	prot_group_download_labels( qq, 0 );
}

void group_update_info( qqclient* qq, qqgroup* g )
{
}


void group_put_event( qqclient* qq )
{
	char *temp;
	int i;
	qqgroup* g;
	NEW( temp, KB(1) );
	if( !temp ) return;
	strcpy( temp, "grouplist^$" );
	pthread_mutex_lock( &qq->group_list.mutex );
	for( i=0; i<qq->group_list.count; i++ ){
		g = (qqgroup*)qq->group_list.items[i];
		sprintf( temp, "%s%u\t%s^@", temp, g->number, g->name );
	}
	pthread_mutex_unlock( &qq->group_list.mutex );
	qqclient_put_event( qq, temp );
	DEL( temp );
}

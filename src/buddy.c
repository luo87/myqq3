/*
 *  buddy.c
 *
 *  Buddy management
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
#ifdef __WIN32__
#include <winsock.h>
#include <wininet.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include "qqclient.h"
#include "memory.h"
#include "debug.h"
#include "protocol.h"
#include "list.h"
#include "buddy.h"
#include "packetmgr.h"


//按qq号码进行升序排序
static int buddy_comp(const void *p, const void *q)
{
	return ( (*(qqbuddy **)p)->number - (*(qqbuddy **)q)->number );
}

static int searcher( const void* p, const void* v )
{
	return ( ((qqbuddy*)p)->number == (int)v );
}

qqbuddy* buddy_get( qqclient* qq, uint number, int create )
{
	if( !number )
		return NULL;
//	if( create && number != qq->number ) return NULL;
	qqbuddy* b;
	b = list_search( &qq->buddy_list, (void*)number, searcher );
	//if not found, b must be NULL
	if( b==NULL && create ){
		NEW( b, sizeof( qqbuddy ) );
		if( !b ) return b;
		b->number = number;
		sprintf( b->nickname, "%u", number );
		if( list_append( &qq->buddy_list, (void*)b )<0 ){
			DEL( b );
		}
	}
	return b;
}

void buddy_remove( qqclient* qq, uint number )
{
	qqbuddy* b;
	b = list_search( &qq->buddy_list, (void*)number, searcher );
	if( b ){
		list_remove( &qq->buddy_list, b );
	}
}

void buddy_sort_list( qqclient* qq )
{
	list_sort( &qq->buddy_list, buddy_comp );
}

static int buddyoff_searcher( const void* p, const void* v )
{
	((qqbuddy*)p)->status = QQ_OFFLINE;
	return 0;
}
void buddy_set_all_off( struct qqclient* qq )
{
	list_search( &qq->buddy_list, NULL, buddyoff_searcher );
}

void buddy_update_list( qqclient* qq )
{
	prot_buddy_update_list( qq, 0 );
	prot_buddy_update_online( qq, 0 );
}

void buddy_update_info( qqclient* qq, qqbuddy* b )
{
	prot_buddy_get_info( qq, b->number );
}


int buddy_send_message( qqclient* qq, uint number, char* msg )
{
	prot_im_send_msg( qq, number, msg );
	return 0;
}

char* buddy_status_string( int st )
{
	switch( st ){
	case QQ_ONLINE:
		return "Online";
	case QQ_OFFLINE:
		return "Offline";
	case QQ_AWAY:
		return "Away";
	case QQ_HIDDEN:
		return "Hidden";
	case QQ_BUSY:
		return "Busy";
	case QQ_KILLME:
		return "KillMe";
	case QQ_QUIET:
		return "Quiet";
	default:
		return "Unknown";
	}
}

//buddy_put_event的函数在登录之后调用比较频繁，
//在获取好友列表，获取好友在线列表，获取好友签名，获取好友备注
void buddy_put_event( qqclient* qq )
{
	char *temp;
	int i;
	qqbuddy* b;
	NEW( temp, KB(128) );
	if( !temp ) return;
	strcpy( temp, "buddylist^$" );
	pthread_mutex_lock( &qq->buddy_list.mutex );
	struct in_addr addr;
	for( i=0; i<qq->buddy_list.count; i++ ){
		b = (qqbuddy*)qq->buddy_list.items[i];
		addr.s_addr = htonl( b->ip );
		sprintf( temp, "%s%u\t%s\t%s\t%s\t%d\t%s\t%s\t%d^@", temp, b->number, buddy_status_string(b->status), b->nickname,
			b->signature, b->sex, inet_ntoa( addr ), b->alias, b->gid );
	}
	pthread_mutex_unlock( &qq->buddy_list.mutex );
	qqclient_put_event( qq, temp );
	DEL( temp );
}

/*
 *  qun.c
 *
 *  QQ Qun
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  Qun
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
#include "buddy.h"
#include "qun.h"

static int qun_searcher( const void* p, const void* v )
{
	return ( ((qqqun*)p)->number == (int)v );
}

static int member_searcher( const void* p, const void* v )
{
	return ( ((qunmember*)p)->number == (int)v );
}


qunmember* qun_member_get( struct qqclient* qq, qqqun* q, uint number, int create )
{
	if( !number )
		return NULL;
	qunmember* m;
	m = list_search( &q->member_list, (void*)number, member_searcher );
	//if not found, m must be NULL
	if( !m && create ){
		NEW( m, sizeof( qunmember ) );
		if( !m ){
			DBG("Fatal error: qunmember not allocated"); 
			return m;
		}
		m->number = number;
		sprintf( m->nickname, "%u", number );
		if( list_append( &q->member_list, (void*)m )<0 ){
			DEL( m );
			DBG("can't add it to member_list");
		}
	}
	return m;
}

void qun_member_remove( struct qqclient* qq, qqqun* q, uint number )
{
	qunmember* m;
	m = list_search( &q->member_list, (void*)number, member_searcher );
	if( m ){
		list_remove( &q->member_list, m );
	}
}

qqqun* qun_get( struct qqclient* qq, uint number, int create )
{
	if( !number )
		return NULL;
	qqqun* q;
	q = list_search( &qq->qun_list, (void*)number, qun_searcher );
	//if not found, b must be NULL
	if( !q && create ){
		NEW( q, sizeof( qqqun ) );
		if( !q ){
			DBG("Fatal error: qqqun not allocated"); 
			return q;
		}
		q->number = number;
		sprintf( q->name, "%u", number );
		if( list_append( &qq->qun_list, (void*)q )<0 ){
			DEL( q );
		}
		if( q ){
			list_create( &q->member_list, MAX_QUN_MEMBER );
			qunmember* m = qun_member_get( qq, q, qq->number, 1 );
			if( m ){
				strcpy( m->nickname, qq->self->nickname );
				m->status = qq->self->status;
			}
		}
	}
	return q;
}

static int qun_ext_searcher( const void* p, const void* v )
{
	return ( ((qqqun*)p)->ext_number == (int)v );
}
qqqun* qun_get_by_ext( struct qqclient* qq, uint ext_number )
{
	return list_search( &qq->qun_list, (void*)ext_number, qun_ext_searcher );
}

void qun_remove( struct qqclient* qq, uint number )
{
	qqqun* q;
	q = list_search( &qq->qun_list, (void*)number, qun_searcher );
	if( q ){
		list_cleanup( &q->member_list );
		list_remove( &qq->qun_list, q );
	}
}

//kill all quns' members, call this when end.
static int qun_kill_members( const void* p, const void* v )
{
	list_cleanup( &((qqqun*)p)->member_list );
	return 0;
}
void qun_member_cleanup( struct qqclient* qq )
{
	list_search( &qq->qun_list, NULL, qun_kill_members );
}

void qun_update_info( qqclient* qq, qqqun* q )
{
	prot_qun_get_info( qq, q->number, 0 );
}

static int get_all_members( const void* p, const void* v )
{
	uint** k = (uint**)v;
	(**k) = ((qunmember*)p)->number;
	(*k)++;
	return 0;
}
void qun_update_memberinfo( qqclient* qq, qqqun* q )
{
	uint* numbers, *v;
	NEW( numbers, MAX_QUN_MEMBER*sizeof(uint) );
	if( !numbers ) return;
	v = numbers;
	list_search( &q->member_list, (void*)&v, get_all_members );
	int count = ((uint)v - (uint)numbers) / sizeof(uint);
	int send;
	v = numbers;
	while( count > 0 ){
		send = 30;	//don't larger than 30, specified in prot_qun.c
		if( count < send ) send = count;
		prot_qun_get_memberinfo( qq, q->number, v, send );
		v += send;
		count -= send;
	}
	DEL( numbers );
}

void qun_update_online( qqclient* qq, qqqun* q )
{
	prot_qun_get_online( qq, q->number );
}

static int memberoff_searcher( const void* p, const void* v )
{
	((qunmember*)p)->status = QQ_OFFLINE;
	return 0;
}
void qun_set_members_off( struct qqclient* qq, qqqun* q )
{
	list_search( &q->member_list, NULL, memberoff_searcher );
}


int qun_send_message( qqclient* qq, uint number, char* msg )
{
	prot_qun_send_msg( qq, number, msg );
	return 0;
}

void qun_put_single_event( qqclient* qq, qqqun* q )
{
	char *temp;
	int i;
	NEW( temp, KB(128) );
	if( !temp )
		return;
	qunmember* m;
	sprintf( temp, "clusterinfo^$%u^$%s^$%s^$%s^$", q->ext_number, q->name, q->ann, q->intro );
	pthread_mutex_lock( &q->member_list.mutex );
	for( i=0; i<q->member_list.count; i++ ){
		m = (qunmember*)q->member_list.items[i];
		sprintf( temp, "%s%u\t%s\t%s\t%d^@", temp, m->number, m->nickname, buddy_status_string( m->status ),
			m->role );
	}
	pthread_mutex_unlock( &q->member_list.mutex );
	qqclient_put_event( qq, temp );
	DEL( temp );
}

void qun_put_event( qqclient* qq )
{
	int i;
	qqqun* q;
	pthread_mutex_lock( &qq->qun_list.mutex );
	for( i=0; i<qq->qun_list.count; i++ ){
		q = (qqqun*)qq->qun_list.items[i];
		qun_put_single_event( qq, q );
	}
	pthread_mutex_unlock( &qq->qun_list.mutex );
}

//更新所有的群信息2009-1-25 11:58
//Update all qun information
static int qun_update_searcher( const void* p, const void* v )
{
	qun_update_info( (struct qqclient*)v, (qqqun*)p );
	return 0;
}
void qun_update_all( struct qqclient* qq )
{
	list_search( &qq->qun_list, (void*)qq, qun_update_searcher );
}

static int qun_update_online_searcher( const void* p, const void* v )
{
	qun_update_online( (struct qqclient*)v, (qqqun*)p );
	return 0;
}
void qun_update_online_all( struct qqclient* qq )
{
	list_search( &qq->qun_list, (void*)qq, qun_update_online_searcher );
}

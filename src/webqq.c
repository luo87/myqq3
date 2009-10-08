/*
 *  webqq.c
 *
 *  WebQQ Program
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-9 10:23:51 Created.
 *
 *  Description: This file mainly includes the functions about 
 *
 */

#define WEBQQLIB

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#ifdef __WIN32__
#include <winsock.h>
#include <wininet.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "debug.h"
#include "memory.h"
#include "qqclient.h"
#include "webqq.h"
#include "loop.h"
#include "buddy.h"
#include "qun.h"
#include "group.h"
#include "config.h"
#include "qqsocket.h"

#define TIMEOUT 120
#define MAX_USERS 1000

static loop user_loop;

static pthread_t thread_guarder;
static int webqq_running = 0;

static void delete_func(const void *p)
{
	user* u = (user*)p;
	qqclient_logout( u->qq );
	qqclient_cleanup( u->qq );
	SLEEP(1);
	DEL( u->qq );
	DEL( u );
}

static int guarder_searcher( const void* p, const void* v )
{
	user* u = (user*)p;
	time_t timenow = (time_t)v;
	if( timenow-u->update_time > TIMEOUT && u->reference == 0 ){	//if timeout
		return 1;
	}
	return 0;
}
static void* webqq_guarder( void* data )
{
	int counter = 0;
	DBG("webqq_guarder");
	while( webqq_running ){
		time_t timenow = time(NULL);
		counter ++;
		user* u;
		do{
			u = loop_search( &user_loop, (void*)timenow, guarder_searcher );
			if( u ){
				DBG("removing qq: %u  time1: %u - time2: %u > 120", 
					((qqclient*)u->qq)->number, timenow, u->update_time );
				loop_remove( &user_loop, (void*)u );
				delete_func( (void*)u );
			}
		}while( u );
		SLEEP( 1 );
	}
	DBG("end.");
	return NULL;
}


EXPORT void webqq_init()
{
	int ret;
	webqq_running = 1;
	config_init();
//	qqsocket_init();
	loop_create( &user_loop, MAX_USERS, delete_func );
	ret = pthread_create( &thread_guarder, NULL, webqq_guarder, (void*)0 );
}

EXPORT void webqq_end()
{
	DBG("webqq_ending");
	webqq_running = 0;
	pthread_join( thread_guarder, NULL );
	loop_cleanup( &user_loop );
	config_end();
//	qqsocket_end();
	DBG("webqq_ended");
}



static int get_user_searcher( const void* p, const void* v )
{
	return ((user*)p)->session_ptr == v;
}
EXPORT user* webqq_get_user( void* session_ptr )
{
	user* u = (user*)loop_search( &user_loop, session_ptr, get_user_searcher );
	if( u ){
		u->reference ++;
		u->update_time = time(NULL);
//		printf("[%u:%u]", (uint)u->update_time, ((qqclient*)u->qq)->number );
	}
	return u;
}

EXPORT user* webqq_create_user( void* session_ptr, uint number, uchar* md5pass )
{
	user* u;
	qqclient* qq;
	u = loop_search( &user_loop, session_ptr, get_user_searcher );
	if( u )
		return u;
	NEW( qq, sizeof(qqclient) );
	NEW( u, sizeof(user) );
	if( !u || !qq ){
		DEL( qq );	DEL( u );
		return NULL;
	}
	u->session_ptr = session_ptr;
	u->create_time = u->update_time = time(NULL);
	u->qq = qq;
	u->reference = 1;
	qqclient_md5_create( qq, number, md5pass );
	qq->auto_accept = 1;	//temporarily do this
	loop_push_to_tail( &user_loop, u );
	return u;
}

//remember to call this function everytime after you use the user.
EXPORT void webqq_derefer( user* u )
{
	u->reference --;
}

EXPORT int webqq_login( user* u )
{
	return qqclient_login( u->qq );
}

EXPORT int webqq_logout( user* u )
{
	DBG("logout %u", ((qqclient*)u->qq)->number );
	qqclient_logout( u->qq );
	return 0;
}

EXPORT int webqq_recv_msg( user* u, char* buf, int size, int wait )
{
	int ret;
	ret = qqclient_get_event( u->qq, buf, size, wait );
	return ret;
}

EXPORT int webqq_send_msg( user* u, uint to, char* buf, char qun_msg )
{
	if( qun_msg ){
		qqqun* q = qun_get_by_ext( u->qq, to );
		if( q )
			return qun_send_message( u->qq, q->number, buf );
	}else{
		return buddy_send_message( u->qq, to, buf );
	}
	return -1;
}

EXPORT void webqq_update_list( user* u )
{
	buddy_update_list( u->qq );
	group_update_list( u->qq );
}

EXPORT void webqq_verify( user* u, uint code )
{
	qqclient_verify( u->qq, (char*) code );
}


EXPORT void webqq_remove( user* u )
{
	loop_remove( &user_loop, u );
	delete_func( (void*) u );
}

EXPORT void webqq_status( user* u, int st )
{
	qqclient_change_status( u->qq, st );
}


void buddy_msg_callback ( qqclient* qq, uint uid, time_t t, char* msg )
{
	char timestr[24];
	struct tm * timeinfo, * tmnow;
	char* str;
	int len, d1, d2;
	time_t t_now;
	t_now = time(NULL);
  	tmnow = localtime( &t_now );
  	d1 = tmnow->tm_mday;
  	timeinfo = localtime ( &t );
  	d2 = timeinfo->tm_mday;
  	if( d1 != d2 ){
		strftime( timestr, 24, "%Y-%m-%d %H:%M:%S", timeinfo );
	}else{
		strftime( timestr, 24, "%H:%M:%S", timeinfo );
	}
	len = strlen( msg );
	NEW( str, len+64 );
	if( uid == 10000 ){
		sprintf( str, "broadcast^$System^$%s", msg );
	}else{
		sprintf( str, "buddymsg^$%u^$%s^$%s", uid, timestr, msg );
	}
	qqclient_put_message( qq, str );
}

void qun_msg_callback ( qqclient* qq, uint uid, uint int_uid,
	time_t t, char* msg )
{
	qqqun* q;
	char timestr[24];
	struct tm * timeinfo, * tmnow;
	char* str;
	int len, d1, d2;
	time_t t_now;
	t_now = time(NULL);
  	tmnow = localtime( &t_now );
  	d1 = tmnow->tm_mday;
  	timeinfo = localtime ( &t );
  	d2 = timeinfo->tm_mday;
  	if( d1 != d2 ){
		strftime( timestr, 24, "%Y-%m-%d %H:%M:%S", timeinfo );
	}else{
		strftime( timestr, 24, "%H:%M:%S", timeinfo );
	}
	q = qun_get( qq, int_uid, 1 );
	if( !q ){
		DBG("error: q=NULL");
		return;
	}
	len = strlen( msg );
	NEW( str, len+64 );
	sprintf( str, "clustermsg^$%u^$%u^$%s^$%s", q->ext_number, uid, timestr, msg );
	qqclient_put_message( qq, str );
}

EXPORT uint webqq_get_number( user* u )
{
	qqclient* qq = (qqclient*)u->qq;
	char event[16];
	qqclient_set_process( qq, qq->process );
	sprintf( event, "status^$%d", qq->mode );
	qqclient_put_event( qq, event );
	buddy_put_event( qq );
	group_put_event( qq );
	qun_put_event( qq );
	qqclient_set_process( qq, qq->process );
	return qq->number;
}

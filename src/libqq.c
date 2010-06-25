/*
 *  libqq.c
 *
 *  LibQQ Program
 *
 *  Copyright (C) 2009  Huang Guan
 *
 *  2009-5-6 23:23:51 Created.
 *
 *  Description: This file mainly includes the functions about 
 *
 */

#define LIBQQLIB

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
#include "libqq.h"
#include "loop.h"
#include "buddy.h"
#include "qun.h"
#include "group.h"
#include "config.h"
#include "qqsocket.h"
#include "utf8.h"

#define MAX_USERS 1000

static int if_init = 0;

static void* login_ex( void* data )
{
	qqclient* qq = (qqclient*) data;
	pthread_detach(pthread_self());
	DBG("login: %u", qq->number );
	qqclient_login( qq );
	return NULL;
}

EXPORT void libqq_init()
{
	config_init();
	if_init = 1;
	qqsocket_init();
}

EXPORT void libqq_cleanup()
{
	config_end();
	qqsocket_end();
}

EXPORT qqclient* libqq_create( uint number, char* pass )
{
	qqclient* qq;
	if( !if_init )
		libqq_init();
	NEW( qq, sizeof(qqclient) );
	if( !qq ){
		DEL( qq );
		return NULL;
	}
	qqclient_create( qq, number, pass );
	qq->auto_accept = 1;	//temporarily do this
	return qq;
}

EXPORT int libqq_login( qqclient* qq )
{
	int ret;
	pthread_t ptr;
	ret = pthread_create( &ptr, NULL, login_ex, (void*)qq );
	if(	ret != 0 ){
		DBG("thread creation failed. ret=%d", ret );
	}
	return ret;
}

EXPORT int libqq_keepalive( qqclient* qq )
{
	qqclient_keepalive( qq );
	return 0;
}

EXPORT int libqq_logout( qqclient* qq )
{
	DBG("logout %u", qq->number );
	qqclient_logout( qq );
	return 0;
}

EXPORT int libqq_detach( qqclient* qq )
{
	DBG("detach %u", qq->number );
	qqclient_detach( qq );
	return 0;
}

EXPORT int libqq_getmessage( qqclient* qq, char* buf, int size, int wait )
{
	int ret;
	ret = qqclient_get_event( qq, buf, size, wait );
	utf8_to_gb( buf, buf, size );
	return ret;
}

EXPORT int libqq_sendmessage( qqclient* qq, uint to, char* buf, char qun_msg )
{
	char* tmp;
	int len = strlen(buf);
	if( len<1 ) return -2;
	NEW( tmp, len*2 );
	gb_to_utf8( buf, tmp, len*2-1 );
	if( qun_msg ){
		qqqun* q = qun_get_by_ext( qq, to );
		if( q )
			qun_send_message( qq, q->number, tmp );
	}else{
		buddy_send_message( qq, to, tmp );
	}
	DEL( tmp );
	return 0;
}

EXPORT void libqq_updatelist( qqclient* qq )
{
	buddy_update_list( qq );
	group_update_list( qq );
}

// 090622 by Huang Guan. change the type of code from uint to const char*
EXPORT void libqq_verify( qqclient* qq, const char* code )
{
	if( code ){
		qqclient_verify( qq, code );
	}else{
		qqclient_verify( qq, NULL );	//this will make the server change another png
	}
}


EXPORT void libqq_remove( qqclient* qq )
{
	qqclient_cleanup( qq );	//will call qqclient_logout if necessary
	DEL( qq );
}

EXPORT void libqq_status( qqclient* qq, int st, uchar has_camera )
{
	qq->has_camera = has_camera;
	qqclient_change_status( qq, st );
}

EXPORT void libqq_addbuddy( qqclient* qq, uint uid, char* msg )
{
	qqclient_add( qq, uid, msg );
}

EXPORT void libqq_delbuddy( qqclient* qq, uint uid )
{
	qqclient_del( qq, uid );
}


void buddy_msg_callback ( qqclient* qq, uint uid, time_t t, char* msg )
{
	char timestr[32];
	struct tm * timeinfo;
	char* str;
	int len;
  	timeinfo = localtime ( &t );
	strftime( timestr, 30, "%Y-%m-%d %H:%M:%S", timeinfo );
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
	char timestr[32];
	struct tm * timeinfo;
	char* str;
	int len;
  	timeinfo = localtime ( &t );
	strftime( timestr, 30, "%Y-%m-%d %H:%M:%S", timeinfo );
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

EXPORT uint libqq_refresh( qqclient* qq )
{
	char event[24];
	qqclient_set_process( qq, qq->process );
	sprintf( event, "status^$%d", qq->mode );
	qqclient_put_event( qq, event );
	buddy_put_event( qq );
	group_put_event( qq );
	qun_put_event( qq );
	qqclient_set_process( qq, qq->process );
	return qq->number;
}

EXPORT void libqq_getqunname( qqclient* qq, uint ext_id, char* buf )
{
	qqqun* q = qun_get_by_ext( qq, ext_id );
	if( q ){
		strncpy( buf, q->name, 15 );
	}else{
		if( ext_id != 0 ){
			sprintf( buf, "%u" , ext_id );
		}
	}
}

EXPORT void libqq_getqunmembername( qqclient* qq, uint ext_id, uint uid, char* buf )
{
	qqqun* q = qun_get_by_ext( qq, ext_id );
	if( q ){
		qunmember* m = qun_member_get( qq, q, uid, 0 );
		if( m ){
			strncpy( buf, m->nickname, 15 );
			return;
		}
	}
	if( uid != 0 ){
		sprintf( buf, "%u" , uid );
	}
}

EXPORT void libqq_getbuddyname( qqclient* qq, uint uid, char* buf )
{
	qqbuddy* b = buddy_get( qq, uid, 0 );
	if( b ){
		strncpy( buf, b->nickname, 15 );
	}else{
		if( uid != 0 ){
			sprintf( buf, "%u" , uid );
		}
	}
}

// 090706 by HG
EXPORT void libqq_sethttpproxy( struct qqclient* qq, char* ip, ushort port )
{
	struct sockaddr_in addr;
	qq->network = PROXY_HTTP;
	netaddr_set( ip, &addr );
	qq->proxy_server_ip = ntohl( addr.sin_addr.s_addr );
	qq->proxy_server_port = port;
}


EXPORT void libqq_getextrainfo( struct qqclient* qq, uint uid )
{
	prot_buddy_get_extra_info( qq, uid );
}

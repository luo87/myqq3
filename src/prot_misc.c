/*
 *  prot_misc.c
 *
 *  QQ Protocol. Part Miscellaneous
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
#include <netdb.h>
#endif

#include "qqclient.h"
#include "memory.h"
#include "debug.h"
#include "qqpacket.h"
#include "packetmgr.h"
#include "qqcrypt.h"
#include "md5.h"
#include "protocol.h"

//41 需要自己验证才被添加为好友
//40 无需验证被添加为好友
//43 主动添加好友成功
void prot_misc_broadcast( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	char  e[4][256];
	char* str = (char*)buf->data;
	int i, len, s, j;
	len = buf->len;
	memset( e, 0, sizeof(e) );
	for( i=0, s=0, j=0; i<=len && j<4; i++ ){
		if( str[i] == 0x1f || i==len ){
			memcpy( e[j], &str[s], MIN(i-s, 255) );
			j++;
			s = i+1;
		}
	}
	if( strcmp( e[0], "06" ) == 0 ){
		//broadcast
		char event[384];
		sprintf( event, "broadcast^$%s^$%s", e[1], e[3] );
		qqclient_put_event( qq, event );
	}else if( strcmp( e[0], "41" ) == 0 ){//still work in qq2009 #被加为好友，要验证
		uint from, to;
		uchar type;
		from = atoi( e[1] );
		to = atoi( e[2] );
		uchar len = e[3][0];
		uchar* p = (uchar*)(&e[3][1]+len);
		type = *p++;
		qqbuddy *b = buddy_get( qq, from, 1 );
		if( b && type == 1 ){
			b->verify_flag = VF_OK;
			if( qq->auto_accept ){
				prot_buddy_verify_addbuddy( qq, 03, from );
			}
		}
		//e[0] e[1] e[2] can be reused 
		if( len > 0 ){
			strncpy( e[0], &e[3][1], len );
			e[0][len] = 0;
		}else{
			strcpy( e[0], "Nothing" );
		}
		sprintf( e[1], "[%u]请求你添加为好友。附言：%s", from, e[0] );
		buddy_msg_callback( qq, 101, time(NULL), e[1] );
	}else if( strcmp( e[0], "04" ) == 0 ){
		uint from, to;
		from = atoi( e[1] );
		to = atoi( e[2] );
		uchar len = e[3][0];
		//e[0] e[1] e[2] can be reused 
		if( len > 0 ){
			strncpy( e[0], &e[3][1], len );
			e[0][len] = 0;
		}else{
			strcpy( e[0], "Nothing" );
		}
		sprintf( e[1], "[%u]拒绝你添加为好友。附言：%s", from, e[0] );
		buddy_msg_callback( qq, 100, time(NULL), e[1] );
	}else if( strcmp( e[0], "40" ) == 0 ){  //#被加为好友，无需验证
		uint from;
		from = atoi( e[1] );
		//e[0] e[1] e[2] can be reused 
		sprintf( e[1], "[%u]已经把你添加为好友。", from );
		buddy_msg_callback( qq, 101, time(NULL), e[1] );
	}else if( strcmp( e[0], "43" ) == 0 ){  //#主动加友成功
		uint from, to;
		from = atoi( e[1] );
		to = atoi( e[2] );
		//refresh buddylist
		buddy_update_list( qq );
		sprintf( e[1], "[%u]已经把你添加为好友！", from );
		buddy_msg_callback( qq, 100, time(NULL), e[1] );
	}else if( strcmp( e[0], "03" ) == 0 ){
		uint from, to;
		from = atoi( e[1] );
		to = atoi( e[2] );
		//refresh buddylist
		buddy_update_list( qq );
		sprintf( e[1], "你已经把[%u]添加为好友！", from );
		buddy_msg_callback( qq, 100, time(NULL), e[1] );
	}else{
		DBG("e[0]: %s", e[0] );
	}
}

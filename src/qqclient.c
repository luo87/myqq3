/*
 *  qqclient.c
 *
 *  QQ Client. 
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *  2008-10-26 TCP UDP Server Infos are loaded from configuration file.
 *
 *  Description: This file mainly includes the functions about 
 *  loading configuration, connecting server, guard thread, basic interface
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __WIN32__
#include <winsock.h>
#include <wininet.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "md5.h"
#include "memory.h"
#include "debug.h"
#include "config.h"
#include "qqsocket.h"
#include "packetmgr.h"
#include "protocol.h"
#include "qqclient.h"
#include "qun.h"
#include "group.h"
#include "buddy.h"
#include "util.h"
#include "qqconn.h"


static void read_config( qqclient* qq )
{
	assert( g_conf );
	qqconn_load_serverlist();
	qq->log_packet = config_readint( g_conf, "QQPacketLog" );
	if( config_readstr( g_conf, "QQNetwork" ) ){
		if( stricmp( config_readstr( g_conf, "QQNetwork" ), "TCP" ) == 0 )
			qq->network = TCP;
		else if( stricmp( config_readstr( g_conf, "QQNetwork" ), "PROXY_HTTP" ) == 0 )
			qq->network = PROXY_HTTP;
		else
			qq->network = UDP;
	}
	if( config_readstr( g_conf, "QQVerifyDir" ) )
		strncpy( qq->verify_dir, config_readstr( g_conf, "QQVerifyDir" ), PATH_LEN );
	if( qq->verify_dir == NULL )
		strcpy( qq->verify_dir, "./web/verify" );
	mkdir_recursive( qq->verify_dir );
}

int qqclient_create( qqclient* qq, uint num, char* pass )
{
	uchar md5_pass[16];
	//加密密码
	md5_state_t mst;
	md5_init( &mst );
	md5_append( &mst, (md5_byte_t*)pass, strlen(pass) );
	md5_finish( &mst, (md5_byte_t*)md5_pass );
	return qqclient_md5_create( qq, num, md5_pass );
}


static void delete_func(const void *p)
{
	if(p)
		DEL( p );
}

int qqclient_md5_create( qqclient* qq, uint num, uchar* md5_pass )
{
	md5_state_t mst;
	//make sure all zero
	memset( qq, 0, sizeof( qqclient ) );
	qq->number = num;
	memcpy( qq->md5_pass1, md5_pass, 16 );
	//make md5_pass2
	md5_init( &mst );
	md5_append( &mst, (md5_byte_t*)qq->md5_pass1, 16 );
	md5_finish( &mst, (md5_byte_t*)qq->md5_pass2 );
	//make md5_pass_qq for qq2011 or newer version
	uchar source[24] = {0};
	memcpy( source, qq->md5_pass1, 16 );
	*(uint*)( &source[20] ) = htonl( qq->number );
	md5_init( &mst );
	md5_append( &mst, (md5_byte_t*)source, 24 );
	md5_finish( &mst, (md5_byte_t*)qq->md5_pass_qq );
	
	qq->mode = QQ_ONLINE;
	qq->process = P_INIT;
	read_config( qq );
	qq->version = QQ_VERSION;
	//
	list_create( &qq->buddy_list, MAX_BUDDY );
	list_create( &qq->qun_list, MAX_QUN );
	list_create( &qq->group_list, MAX_GROUP );
	loop_create( &qq->event_loop, MAX_EVENT, delete_func );
	loop_create( &qq->msg_loop, MAX_EVENT, delete_func );
	pthread_mutex_init( &qq->mutex_event, NULL );
	//create self info
	qq->self = buddy_get( qq, qq->number, 1 );
	if( !qq->self ){
		DBG("[%d] Fatal error: qq->self == NULL", qq->number);
		return -1;
	}
	return 0;
}


#define INTERVAL 1
void qqclient_keepalive(qqclient* qq)
{
	int counter = qq->keep_alive_counter ++;
	if( qq->process == P_LOGGING || qq->process == P_LOGIN || qq->process == P_VERIFYING ){
		packetmgr_check_packet( qq, 5 );
		if( qq->process == P_LOGIN ){
			//1次心跳/分钟
			if( counter % ( 1 *30*INTERVAL) == 0 ){
				prot_user_keep_alive( qq );
			}
#ifndef NO_BUDDY_INFO
			//10分钟刷新在线列表 QQ2009是5分钟刷新一次。
			if( counter % ( 10 *60*INTERVAL) == 0 ){
				prot_buddy_update_online( qq, 0 );
				qun_update_online_all( qq );
			}
#endif
			//30分钟刷新状态和刷新等级
			if( counter % ( 30 *60*INTERVAL) == 0 ){
				prot_user_change_status( qq );
				prot_user_get_level( qq );
			}
		//	//等待登录完毕
			if( qq->login_finish==0 ){
				if( loop_is_empty(&qq->packetmgr.ready_loop) && 
					loop_is_empty(&qq->packetmgr.sent_loop) ){
					qq->login_finish = 1;	//we can recv message now.
				}
			}
			qq->online_clock ++;
		}
	}
}

#ifndef NO_KEEPALIVE
static void* qqclient_keepalive_proc( void* data )
{
	qqclient* qq = (qqclient*) data;
	int sleepintv = 1000/INTERVAL;
	int counter = 0;
	DBG("keepalive");
	while( qq->process != P_INIT ){
		counter ++;
		if( counter % INTERVAL == 0 ){
			qqclient_keepalive(qq);
		}
		USLEEP( sleepintv );
	}
	DBG("end.");
	return NULL;
}
#endif

int qqclient_login( qqclient* qq )
{
	DBG("login");
	int ret;
	if( qq->process != P_INIT ){
		if( qq->process == P_WAITING ){
			qqclient_set_process( qq, P_LOGGING );
			prot_login_verify( qq );
			return 0;
		}
		DBG("please logout first");
		return -1;
	}
	if( qq->login_finish == 2 ){
		qqclient_logout( qq );
	}
	qqclient_set_process( qq, P_LOGGING );
	srand( qq->number + time(NULL) );
	//start packetmgr
	packetmgr_start( qq );
	packetmgr_new_seqno( qq );
	qqconn_get_server( qq );
	ret = qqconn_connect( qq );
	if( ret < 0 ){
		qqclient_set_process( qq, P_ERROR );
		return ret;
	}
	//ok, already connected to the server
	if( qq->network == PROXY_HTTP ){
		qqconn_establish( qq );
	}else{
		//send touch packet
		prot_login_touch( qq );
	}
	//keep
	qq->keep_alive_counter = 0;
#ifndef NO_KEEPALIVE
	ret = pthread_create( &qq->thread_keepalive, NULL, qqclient_keepalive_proc, (void*)qq );
#endif
	return 0;
}

void qqclient_detach( qqclient* qq )
{
	if( qq->process == P_INIT )
		return;
	if( qq->process == P_LOGIN ){
	//	int i;
	//	for( i = 0; i<4; i++ )
	//		prot_login_logout( qq );
	}else{
		DBG("process = %d", qq->process );
	}
	qq->login_finish = 2;
	qqclient_set_process( qq, P_INIT );
	qqsocket_close( qq->http_sock );
	qqsocket_close( qq->socket );
}

//该函数需要等待延时，为了避免等待，可以预先执行detach函数
void qqclient_logout( qqclient* qq )
{
	if( qq->login_finish != 2 ){ 	//未执行过detach
		if( qq->process == P_INIT )
			return;
		qqclient_detach( qq );
	}
	qq->login_finish = 0;
	DBG("joining keepalive");
#ifndef NO_KEEPALIVE
	#ifdef __WIN32__
	pthread_join( qq->thread_keepalive, NULL );
	#else
	if( qq->thread_keepalive )
		pthread_join( qq->thread_keepalive, NULL );
	#endif
#endif
	packetmgr_end( qq );
}


void qqclient_cleanup( qqclient* qq )
{
	if( qq->login_finish == 2 )
		qqclient_logout( qq );
	if( qq->process != P_INIT )
		qqclient_logout( qq );
	pthread_mutex_lock( &qq->mutex_event );
	qun_member_cleanup( qq );
	list_cleanup( &qq->buddy_list );
	list_cleanup( &qq->qun_list );
	list_cleanup( &qq->group_list );
	loop_cleanup( &qq->event_loop );
	loop_cleanup( &qq->msg_loop );
	pthread_mutex_destroy( &qq->mutex_event );
}

int qqclient_verify( qqclient* qq, const char* code )
{
	if( qq->login_finish == 1 ){
		qqclient_set_process( qq, P_LOGIN );	//原来漏了这个  20090709
		prot_user_request_token( qq, qq->data.operating_number, qq->data.operation, 1, code );
	}else{
		qqclient_set_process( qq, P_LOGGING );	//原来这个是P_LOGIN，错了。 20090709
		prot_login_request( qq, &qq->data.verify_token, code, 0 );
	}
	DBG("verify code: %x", code );
	return 0;
}

int qqclient_add( qqclient* qq, uint number, char* request_str )
{
	qqbuddy* b = buddy_get( qq, number, 0 );
	if( b && b->verify_flag == VF_OK )	{
		prot_buddy_verify_addbuddy( qq, 03, number );	//允许一个请求x
	}else{
		strncpy( qq->data.addbuddy_str, request_str, 50 );
		prot_buddy_request_addbuddy( qq, number );
	}
	return 0;
}

int qqclient_del( qqclient* qq, uint number )
{
	qq->data.operating_number = number;
	prot_user_request_token( qq, number, OP_DELBUDDY, 6, 0 );
	return 0;
}

int qqclient_wait( qqclient* qq, int sec )
{
	int i;
	//we don't want to cleanup the data while another thread is waiting here.
	if( pthread_mutex_trylock( &qq->mutex_event ) != 0 )
		return -1;	//busy?
	for( i=0; (sec==0 || i<sec) && qq->process!=P_INIT; i++ ){
		if( loop_is_empty(&qq->packetmgr.ready_loop) && loop_is_empty(&qq->packetmgr.sent_loop) )
		{
			pthread_mutex_unlock( &qq->mutex_event );
			return 0;
		}
		SLEEP(1);
	}
	pthread_mutex_unlock( &qq->mutex_event );
	return -1;
}

void qqclient_change_status( qqclient* qq, uchar mode )
{
	qq->mode = mode;
	if( qq->process == P_LOGIN ){
		prot_user_change_status( qq );
	}
}

// wait: <0   wait until event arrives
// wait: 0  don't need to wait, return directly if no event
// wait: n(n>0) wait n secondes.
// return: 1:ok  0:no event  -1:error
int qqclient_get_event( qqclient* qq, char* event, int size, int wait )
{
	char* buf;
	//we don't want to cleanup the data while another thread is waiting here.
	if( pthread_mutex_trylock( &qq->mutex_event ) != 0 )
		return -1;	//busy?
	for( ; ; ){
		if(  qq->process == P_INIT ){
			pthread_mutex_unlock( &qq->mutex_event );
			return -1;
		}
		buf = loop_pop_from_head( &qq->event_loop );
		if( buf ){
			int len = strlen( buf );
			if( len < size ){
				strcpy( event, buf );
			}else{
				strncpy( event, buf, size-1 ); //strncpy的n个字符是否包括'\n'？？？
				DBG("buffer too small.");
			}
			delete_func( buf );
			pthread_mutex_unlock( &qq->mutex_event );
			return 1;
		}
		if( qq->online_clock > 10 ){
			buf = loop_pop_from_head( &qq->msg_loop );
			if( buf ){
				int len = strlen( buf );
				if( len < size ){
					strcpy( event, buf );
				}else{
					strncpy( event, buf, size-1 ); //strncpy的n个字符是否包括'\n'？？？
					DBG("buffer too small.");
				}
				delete_func( buf );
				pthread_mutex_unlock( &qq->mutex_event );
				return 1;
			}
		}
		if( wait<0 || wait> 0 ){
			if( wait>0) wait--;
			USLEEP( 200 );
		}else{
			break;
		}
	}
	pthread_mutex_unlock( &qq->mutex_event );
	return 0;
}

int qqclient_put_event( qqclient* qq, char* event )
{
	char* buf;
	int len = strlen( event );
	NEW( buf, len+1 );
	if( !buf ) return -1;
	strcpy( buf, event );
	loop_push_to_tail( &qq->event_loop, (void*)buf );
	return 0;
}

int qqclient_put_message( qqclient* qq, char* msg )
{
	char* buf;
	int len = strlen( msg );
	NEW( buf, len+1 );
	if( !buf ) return -1;
	strcpy( buf, msg );
	loop_push_to_tail( &qq->msg_loop, (void*)buf );
	return 0;
}



void qqclient_set_process( qqclient *qq, int process )
{
	qq->process = process;
	char event[24];
	sprintf( event, "process^$%d", process );
	qqclient_put_event( qq, event );
}

#ifndef _QQCLIENT_H
#define _QQCLIENT_H

#include <pthread.h>
#include <time.h>
#include "qqdef.h"
#include "packetmgr.h"
#include "buddy.h"
#include "list.h"
#include "loop.h"
#include "qqsocket.h"
#include "protocol.h"

//login process
enum LOGIN_PROCESS{
	P_INIT = 0,
	P_LOGGING,
	P_VERIFYING,
	P_LOGIN,
	P_ERROR,
	P_DENIED,
	P_WRONGPASS,
	P_BUSY
};


typedef struct server_item{
	char	ip[32];
	ushort	port;
}server_item;

typedef struct qqclient{
	uint		number;
	ushort		version;
	ushort		seqno;
	int			socket;
	char		password[32];
	uchar		md5_pass1[16];
	uchar		md5_pass2[16];
	uint		server_ip;
	ushort		server_port;
	uint		proxy_server_ip;
	ushort		proxy_server_port;
	uchar		proxy_established;
	uint		client_ip;
	ushort		client_port;
	uint		local_ip;
	ushort		local_port;
	uint		last_login_ip;
	uint		last_login_time;
	uint		login_time;
	uint		server_time;
	char		mode;
	char		has_camera;
	int			process;
	char		log_terminal;
	char		log_packet;
	pthread_t	thread_keepalive;
	qqpacketmgr	packetmgr;
	qqbuddy*	self;
	//buddy qun group
	list		buddy_list;
	list		qun_list;
	list		group_list;
	uint		online_clock;
	char		verify_dir[PATH_LEN];
	ushort		level;
	ushort		active_days;
	ushort		upgrade_days;
	loop		event_loop;
	loop		msg_loop;
	pthread_mutex_t	mutex_event;
	char		network;	//TCP or UDP
	char		login_finish;
	char		auto_accept;	//accept addbuddy requests
	char		auto_reply[AUTO_REPLY_LEN];
	int			http_sock;
	union{
		struct qqdata_2009	data;
	};
}qqclient;


int qqclient_create( qqclient* qq, uint num, char* pass ); 
int qqclient_md5_create( qqclient* qq, uint num, uchar* md5_pass ); 
int qqclient_login( qqclient* qq );
void qqclient_logout( qqclient* qq );
void qqclient_detach( qqclient* qq );
void qqclient_cleanup( qqclient* qq );
int qqclient_verify( qqclient* qq, uint code );
int qqclient_wait( qqclient* qq, int sec );
void qqclient_change_status( qqclient* qq, uchar mode );
int qqclient_get_event( qqclient* qq, char* event, int size, int wait );
int qqclient_put_event( qqclient* qq, char* event );
void qqclient_set_process( qqclient *qq, int process );
int qqclient_put_message( qqclient* qq, char* msg );
void qqclient_get_server( qqclient* qq );
int qqclient_add( qqclient* qq, uint number, char* request_str );
int qqclient_del( qqclient* qq, uint number );

#endif

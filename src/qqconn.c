/*
 *  qqconn.c
 *
 *  QQ Connections. 
 *
 *  Copyright (C) 2009  Huang Guan
 *
 *  2009-7-4 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  connecting server
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

#include "memory.h"
#include "debug.h"
#include "config.h"
#include "qqsocket.h"
#include "protocol.h"
#include "qqclient.h"
#include "util.h"
#include "qqconn.h"

static server_item tcp_servers[MAX_SERVER_ADDR];
static server_item udp_servers[MAX_SERVER_ADDR];
static server_item proxyhttp_servers[MAX_SERVER_ADDR];
static int tcp_server_count = 0, udp_server_count = 0, proxyhttp_server_count = 0;

static uint last_server_ip = 0, last_server_port = 0;	//for quick login

// 解析服务器列表
static void read_server_addr( server_item* srv, char* s, int* count  )
{
	char ip[32], port[10], read_name = 1, *p;
	int j = 0;
	for( p=s; ; p++ ){
		if( *p == ':' ){
			ip[j]=0;
			j=0; read_name = 0;
		}else if( *p=='|' || *p=='\0' ){
			port[j]=0;
			j=0; read_name = 1;
			if( *count < MAX_SERVER_ADDR ){
				strncpy( srv[*count].ip, ip, 31 );
				srv[*count].port = atoi( port );
//				printf("%s:%d\n", srv[*count].ip, srv[*count].port );
				(*count) ++;
			}
			if( *p=='\0' )
				break;
		}else if(*p==' '){
			continue;
		}else{
			if( read_name ){
				if( j<31 )	ip[j++] = *p;
			}else{
				if( j<9 ) port[j++] = *p;
			}
		}
	}
}

// 从配置文件读入服务器列表
void qqconn_load_serverlist()
{
	if( !tcp_server_count && !udp_server_count ){
		char* tcps, *udps, *proxy_https;
		tcps = config_readstr( g_conf, "QQTcpServerList" );
		udps = config_readstr( g_conf, "QQUdpServerList" );
		proxy_https = config_readstr( g_conf, "QQHttpProxyServerList" );
		if( tcps ){
			read_server_addr( tcp_servers, tcps, &tcp_server_count );
		}
		if( udps ){
			read_server_addr( udp_servers, udps, &udp_server_count );
		}
		if( proxy_https ){
			read_server_addr( proxyhttp_servers, proxy_https, &proxyhttp_server_count );
		}
	}
}

// 获取服务器地址
void qqconn_get_server(qqclient* qq)
{
	int i;
	struct sockaddr_in addr;
	if( last_server_ip == 0 ){
		//random an ip
		if( qq->network == TCP && tcp_server_count>0 ){
			i = rand()%tcp_server_count;
			netaddr_set( tcp_servers[i].ip, &addr );
			qq->server_ip = ntohl( addr.sin_addr.s_addr );
			qq->server_port = tcp_servers[i].port;
		}else if( qq->network == PROXY_HTTP && tcp_server_count>0 && proxyhttp_server_count>0 ){
			//Setup Proxy Server
			if( !qq->proxy_server_ip || !qq->proxy_server_port ){
				i = rand()%proxyhttp_server_count;
				netaddr_set( proxyhttp_servers[i].ip, &addr );
				qq->proxy_server_ip = ntohl( addr.sin_addr.s_addr );
				qq->proxy_server_port = proxyhttp_servers[i].port;
			}
			//Setup QQ TCP Server
			i = rand()%tcp_server_count;
			netaddr_set( tcp_servers[i].ip, &addr );
			qq->server_ip = ntohl( addr.sin_addr.s_addr );
			qq->server_port = tcp_servers[i].port;
		}else{
			if( udp_server_count <1 ){
				qq->process = P_ERROR;
				DBG("no server for logging.");
				return;
			}
			i = rand()%udp_server_count;
			netaddr_set( udp_servers[i].ip, &addr );
			qq->server_ip = ntohl( addr.sin_addr.s_addr );
			qq->server_port = udp_servers[i].port;
		}
	}else{
		qq->server_ip = last_server_ip;
		qq->server_port = last_server_port;
	}
}

// 更换qq服务器
static char proxy_format[] = "CONNECT %s:%d HTTP/1.1\r\n"
"Accept: */*\r\n"
"Content-Type: text/html\r\n"
"Proxy-Connection: Keep-Alive\r\n"
"Content-length: 0\r\n\r\n";
int qqconn_establish( qqclient* qq )
{
	char tmp[256];
	int ret;
	if( qq->network != PROXY_HTTP ){
		DBG("## Warning: not in proxy mode.");
		return -1;
	}
	struct in_addr addr;
	addr.s_addr = htonl( qq->server_ip );
	sprintf( tmp, proxy_format, inet_ntoa( addr ), qq->server_port );
	ret = qqsocket_send( qq->socket, tmp, strlen(tmp) );
	if( ret<=0 ){
		DBG("failed to send data to proxy sever.");
		qqclient_set_process( qq, P_ERROR );
		return -2;
	}
	qq->proxy_established = 0;
	return 0;
}

// 连接服务器
int qqconn_connect( qqclient* qq )
{
	//connect server
	uint ip;
	ushort port;
	if( qq->socket )
		qqsocket_close( qq->socket );
	if( qq->network == TCP ){
		qq->socket = qqsocket_create( TCP, NULL, 0 );
		ip = qq->server_ip;
		port = qq->server_port;
	}else if( qq->network == PROXY_HTTP ){
		qq->socket = qqsocket_create( TCP, NULL, 0 );
		ip = qq->proxy_server_ip;
		port = qq->proxy_server_port;
	}else{
		qq->socket = qqsocket_create( UDP, NULL, 0 );
		ip = qq->server_ip;
		port = qq->server_port;
	}
	if( qq->socket < 0 ){
		DBG("can't not create socket. ret=%d", qq->socket );
		return -1;
	}
	struct in_addr addr;
	addr.s_addr = htonl( ip );
	DBG("connecting to %s:%d", inet_ntoa( addr ), port );
	if( qqsocket_connect2( qq->socket, ip, port ) < 0 ){
		DBG("can't not connect server %s", inet_ntoa( addr ) );
		return -1;
	}
	return 0;
}


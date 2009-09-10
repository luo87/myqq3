/*
 *  qqsocket.c
 *
 *  Some functions to use the POSIX socket.
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-01-31 Created.
 *
 *  Description: This file mainly includes the functions about 
 *               qqsocket_create, qqsocket_close
 *               qqsocket_send, qqsocket_recv
 *               qqsocket_connect, qqsocket_connect2
 */


#include <stdio.h>
#include <unistd.h>

#ifdef __WIN32__
#include <winsock.h>
#include <wininet.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qqdef.h"
#include "debug.h"
#include "qqsocket.h"

#ifdef __WIN32__
static WSADATA wsa_data;
#endif

void qqsocket_init()
{
#ifdef __WIN32__
	int ret = WSAStartup(MAKEWORD(2,1), &wsa_data); //初始化WinSocket动态连接库
	if( ret != 0 ) // 初始化失败；
	{
		DBG("failed in WSAStartup");
	}
	DBG("WSA Startup.");
#endif
}
void qqsocket_end()
{
#ifdef __WIN32__
	WSACleanup(); 
#endif
}

//Create a socket TCP or UDP
int qqsocket_create( int type, char* ip, ushort port )
{
	int fd = 0;
	struct sockaddr_in addr;
	switch( type )
	{
	case TCP:
		fd = socket( PF_INET, SOCK_STREAM, 0 );
		memset( &addr, 0, sizeof(struct sockaddr_in) );
		addr.sin_family = PF_INET;
		if( ip )
			netaddr_set( ip, &addr ); //addr.sin_addr.s_addr = inet_addr( ip );
		else
			addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons( port );
		if( bind( fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in) ) < 0 )
		{
			DBG("bind tcp socket error!");
			close( fd );
			return -1;
		}
		break;
	case UDP:
		fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
		memset( &addr, 0, sizeof(struct sockaddr) );
		addr.sin_family = PF_INET;
		if( ip )
			netaddr_set( ip, &addr ); //addr.sin_addr.s_addr = inet_addr( ip );
		else
			addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons( port );
		if( bind( fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in) ) < 0 )
		{
			DBG("bind udp socket error!");
			close( fd );
			return -1;
		}
		break;
	}
	return fd;
}

//close a socket
void qqsocket_close( int fd )
{
#ifdef __WIN32__
	closesocket( fd );
#else
	close( fd );
#endif
} 

//connect a socket to remote server
int qqsocket_connect( int fd, char* ip, ushort port )
{
	struct sockaddr_in addr;
	memset( &addr, 0, sizeof(struct sockaddr_in) );
	addr.sin_family = PF_INET;
	netaddr_set( ip, &addr );
	addr.sin_port = htons( port );
	if( connect( fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0 )
	{
		DBG("qqsocket connect failed.");
		return -1;
	}
	return 0;
}

//connect a socket to remote server
int qqsocket_connect2( int fd, uint ip, ushort port )
{
	struct sockaddr_in addr;
	memset( &addr, 0, sizeof(struct sockaddr_in) );
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = htonl( ip );
	addr.sin_port = htons( port );
	if( connect( fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0 )
	{
		DBG("qqsocket connect failed.");
		return -1;
	}
	return 0;
}

void netaddr_set( char* name, struct sockaddr_in* addr )
{
	if( (addr->sin_addr.s_addr = inet_addr( name ) ) == -1 )
	{
		//it's not an IP.
		//not an ip, maybe a domain
		struct hostent *host;
		host = gethostbyname( name );
		if( host )
		{
			addr->sin_addr.s_addr = *(size_t*) host->h_addr_list[0];
			DBG("Get IP: %s", inet_ntoa( addr->sin_addr ) );
		}else{
			DBG("Failed to get ip by %s", name );
		}
	}	
}

int qqsocket_send( int fd, uchar* buf, size_t size )
{
	int ret;
	size_t rest;
	rest = size;
	while( rest > 0 )
	{
		ret = send( fd, (char*)buf, rest, 0);
		if(ret <= 0 )
		{
			return ret;
		}
		rest -= ret;
		buf += ret;
	}
	return size;
}

int qqsocket_recv( int fd, uchar* buf, size_t size )
{
	int ret;
	ret = recv( fd, (char*)buf, size, 0 );
	return ret;
}



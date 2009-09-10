/*
 *  qqpacket.c
 *
 *  QQ Packet Operation
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  
 *
 */

#include <string.h>
#include <stdlib.h>
#ifdef __WIN32__
#include <winsock.h>
#include <wininet.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include "qqdef.h"
#include "memory.h"
#include "debug.h"
#include "packetmgr.h"
#include "qqclient.h"
#include "qqpacket.h"

void put_byte( bytebuffer* p, uchar b )
{
	if( p->pos<p->size ){
		p->data[p->pos++] = b;
	}else{ 
		DBG("packet p->pos(%d) >= p->size(%d)", p->pos, p->size );
	}
}

void put_word( bytebuffer* p, ushort b )
{
	if( p->pos+1<p->size ){ 
		*(ushort*)(&p->data[p->pos]) = htons(b); p->pos+=2;
	}else{
		DBG("packet p->pos(%d)+1 >= p->size(%d)", p->pos, p->size );
	}
}

void put_int( bytebuffer* p, uint b )
{
	if( p->pos+3<p->size ){ 
		*(uint*)(&p->data[p->pos]) = htonl(b); p->pos+=4;
	}else{
		DBG("packet p->pos(%d)+3 >= p->size(%d)", p->pos, p->size );
	}
}

void put_data( bytebuffer* p, uchar* data, int len )
{
	if( p->pos+len<=p->size ){
		memcpy( &p->data[p->pos], data, len );
		p->pos += len;
	}else{
		DBG("packet p->pos(%d)+%d > p->size(%d)", p->pos, len, p->size );
	}
}

void put_string( bytebuffer* p, char* str )
{
	put_data( p, (uchar*)str, strlen( str ) );
}

uchar get_byte( bytebuffer* p )
{
	uchar b = 0;
	if( p->pos<p->len ){
		b = p->data[p->pos++];
	}else{
		DBG("packet p->pos(%d) >= p->len(%d)", p->pos, p->len );
	}
	return b;
}

ushort get_word( bytebuffer* p )
{
	ushort b = 0;
	if( p->pos+1<p->len ){
		b = *(ushort*)(&p->data[p->pos]); p->pos+=2;
	}else{
		DBG("packet p->pos(%d)+1 >= p->len(%d)", p->pos, p->len );
	}
	return ntohs(b);
}

uint get_int( bytebuffer* p )
{
	uint b = 0;
	if( p->pos+3<p->len ){
		b = *(uint*)(&p->data[p->pos]); p->pos+=4;\
	}else{ 
		DBG("packet p->pos(%d)+3 >= p->len(%d)", p->pos, p->len );
	}
	return ntohl(b);
}

int get_string( bytebuffer* p, char* str, int len )
{
	int j = 0;
	while( p->pos<p->len && j+1<len ){
		str[j++] = p->data[p->pos++];
		if( str[j-1] == '\0' )
			return j;
	}
	str[j] = 0;
	return j;
}

int get_data( bytebuffer* p, uchar* data, int len )
{
	if( p->pos+len<=p->len ){
		memcpy( data, &p->data[p->pos], len );
		p->pos += len;
	}else{ 
		DBG("packet p->pos(%d)+%d > p->len(%d)", p->pos, len, p->len );
		len = 0;
	}
	return len;
}

int get_token( bytebuffer* p, token* tok )
{
	ushort len = get_word( p );
	get_data( p, (uchar*)tok->data, len );
	tok->len = len;
	return len;
}


int get_token2( bytebuffer* p, token* tok )
{
	uchar len = get_byte( p );
	get_data( p, (uchar*)tok->data, len );
	tok->len = len;
	return len;
}

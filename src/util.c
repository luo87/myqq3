/*
 *  util.c
 *
 *  MyQQ utilities
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-15 Created.
 *  2009-3-22 Add msleep.
 *
 */

#ifdef __WIN32__
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "qqsocket.h"
#include "debug.h"
#include "qqdef.h"
#include "util.h"
#include "memory.h"

int mkdir_recursive( char* path )
{
	char *p;
	if( access( path, 0 ) == 0 )
		return 0;
	for( p=path; *p; p++ ){
		if( p>path && *p == '/' ){
			*p = 0;
			if( access( path, 0 ) != 0 ){
#ifdef __WIN32__
				mkdir( path );
#else
				if( mkdir( path, S_IRWXU ) != 0 )
					return -1;
#endif
			}
			*p = '/';
		}
	}
#ifdef __WIN32__
	return mkdir( path );
#else
	return mkdir( path, S_IRWXU );
#endif
}

int trans_faces( char* src, char* dst, int outlen )
{
	char * p, *q;
	p = src;	q = dst;
	while( *p ){
		if( *(p++) == 0x14 ){
			if( (int)(q-dst) < outlen - 10 )
				q += sprintf( q, "[face:%u]", *(p++) );
		}else{
			if( (int)(q-dst) < outlen )
				*(q++) = *(p-1);
		}
	}
	*q = 0;
	return (int)(q-dst);
}

//2009-2-7 9:20 Huang Guan
//get middle value from a string by the left and the right. 
char* mid_value( char* str, char* left, char* right, char* out, int outlen )
{
	char* beg, * end, t;
	beg = strstr( str, left );
	if( beg ){
		beg += strlen(left);
		if( right ){
			end = strstr( beg, right );
		}else{
			end = beg + strlen(beg);
		}
		if( end ){
			t = *end; *end = 0;
			strncpy( out, beg, outlen );
			*end = t;
			return end;	//returns the end
		}
	}
	*out = '\0';
	return str;
}

//2009-2-7 9:21 Huang Guan
//Download a file
//http://verycode.qq.com/getimage?fromuin=942011793&touin=357339036&appt=1&flag=1
//GET  HTTP/1.1
int http_request( int* http_sock, char* url, char* session, char* data, int* datalen )
{
	char host_name[32], uri[101], *header, *next, tmp[16];
	int len, data_len;
	//get host name
	next = mid_value( url, "//", "/", host_name, 31 );
	next = mid_value( next, "/", NULL, uri, 100 );
	//connect
	*http_sock = qqsocket_create( TCP, NULL, 0 );
	if( *http_sock <= 0 )	return -3;
	qqsocket_connect( *http_sock, host_name, 80 );
	NEW( header, KB(4) );
	sprintf( header, "GET %s HTTP/1.1\r\n\r\n", uri );
	qqsocket_send( *http_sock, (uchar*)header, strlen(header) );
	len = qqsocket_recv( *http_sock, (uchar*)header, KB(4) );
//	puts( header );
	if( len>0 ){
		next = mid_value( header, "Content-Length: ", "\r\n", tmp, 15 );
		data_len = atoi( tmp );
		next = mid_value( header, "getqqsession:", "\r\n", session, 127 );
		while( len < data_len ){
			int ret = qqsocket_recv( *http_sock, (uchar*)(header + len), KB(4)-len );
			if( ret > 0 )	len += ret;	else	break;
		}
		next = strstr( header, "\r\n\r\n" ) + 4;
		if( next ){
			if( *datalen > data_len ){ 
				memcpy( (void*)data, (void*)next, data_len );
				*datalen = data_len;
			}else{
				DBG("datalen is too small.");
			}
		}
	}
	DEL( header );
	qqsocket_close( *http_sock );
	return 0;
}

#ifdef __WIN32__
void msleep( unsigned int ms )
{
	Sleep( ms );
}
#endif

int get_splitable_pos( char* buf, int pos )
{
	//pos = 699
	if( (uchar)buf[pos]>=0x80 && (uchar)buf[pos]<=0xBF ){
		do	
			pos--;
		while( pos && (uchar)buf[pos]<0xC2 );
	}
	return pos;	//buf[pos]不可取
}

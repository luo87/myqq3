#ifndef _QQSOCKET_H
#define _QQSOCKET_H


#include "qqdef.h"

struct sockaddr_in;

int qqsocket_create( int type, char* ip, ushort port );
int qqsocket_connect( int fd, char* ip, ushort port );
int qqsocket_connect2( int fd, uint ip, ushort port );
int qqsocket_send( int fd, uchar* buf, size_t size );
int qqsocket_recv( int fd, uchar* buf, size_t size );
void qqsocket_close( int fd );
void netaddr_set( char* name, struct sockaddr_in* addr );
void qqsocket_init();
void qqsocket_end();

#endif //_QQSOCKET_

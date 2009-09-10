#ifndef _PACKET_H
#define _PACKET_H

#include "qqdef.h"

#define PACKET_SIZE KB(4)

typedef struct bytebuffer{
	int pos;
	int len;
	int size;
	uchar data[PACKET_SIZE];
}bytebuffer;

typedef struct qqpacket{
//	struct qqpacket* next;
//	struct qqpacket* pre;
	char		head;
	char		tail;
	ushort		version;
	ushort		command;
	ushort		seqno;
	bytebuffer*	buf;
	time_t		time_create;
	time_t		time_alive;	//send or recv
	struct qqpacket* match;
	int		send_times;
	char		key_type;
	uchar		key[16];	//key for encrypt or decrypt
	char		need_ack;
}qqpacket;

void put_byte( bytebuffer* p, uchar b );
void put_word( bytebuffer* p, ushort b );
void put_int( bytebuffer* p, uint b );
void put_data( bytebuffer* p, uchar* data, int len );
void put_string( bytebuffer* p, char* str );

uchar get_byte( bytebuffer* p );
ushort get_word( bytebuffer* p );
uint get_int( bytebuffer* p );
int get_string( bytebuffer* p, char* str, int len );
int get_data( bytebuffer* p, uchar* data, int len );
struct token;
int get_token( bytebuffer* p, token* data );
int get_token2( bytebuffer* p, token* tok );

#endif


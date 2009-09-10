/*
 *  prot_user.c
 *
 *  QQ Protocol. Part User Informaion
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
#include "protocol.h"
#include "buddy.h"
#include "util.h"

void prot_user_keep_alive( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_KEEP_ALIVE );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	char num_str[16];
	sprintf( num_str, "%u", qq->number );
	put_data( buf, (void*)num_str, strlen(num_str) );
	post_packet( qq, p, SESSION_KEY );
}

void prot_user_keep_alive_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	get_byte( buf );	//00
	int onlines;
	onlines = get_int( buf );
	int ip;
	ip = get_int( buf ); //client ip
	ushort port = get_word( buf ); //client port
	get_word( buf );	//unknown 00 3c
	uint server_time;
	server_time = get_int( buf );
	//...5 zeros 
	time_t t;
	t = CN_TIME( server_time );
	char event[64];
	sprintf( event, "keepalive^$%u", qq->number );
	qqclient_put_event( qq, event );
//	DBG("keepalive: %u ", qq->number );
	port = ip= 0;
}

void prot_user_change_status( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_CHANGE_STATUS );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, qq->mode );
	put_int( buf, 0 );
	put_int( buf, 1 );	//camera??
	put_word( buf, 0 );
	post_packet( qq, p, SESSION_KEY );
}

void prot_user_change_status_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	if( get_byte( buf ) == '0' ){
		qq->self->status = qq->mode;
		DBG("change status to %d", qq->mode );
		char event[16];
		sprintf( event, "status^$%d", qq->mode );
		qqclient_put_event( qq, event );
	}else{
		DBG("change status failed.");
	}
}


void prot_user_get_key( struct qqclient* qq, uchar key )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GET_KEY );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, key );
	post_packet( qq, p, SESSION_KEY );
}

void prot_user_get_key_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd, result;
	uchar key[16];
	token tok;
	cmd = get_byte( buf );
	result = get_byte( buf );
	if( result != 0 ){
		DBG("faild to get key.");
	}
	switch( cmd ){
	case 4:	//file key
		get_data( buf, key, 16 );
		buf->pos += 12;	
		qq->data.file_token.len = (ushort)get_byte(buf);
		get_data(buf,qq->data.file_token.data, qq->data.file_token.len); 
//		get_token( buf, &tok );
		memcpy( qq->data.file_key, key, 16 );
		qq->data.file_token = tok;
		DBG("got file key.");
		break;
	default:
		DBG("got unknown key.");
	}
}

void prot_user_get_notice( struct qqclient* qq, uchar type )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GET_KEY );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	p->need_ack = 0;
	switch( type ){
	case 0:
		put_int( buf, qq->number );
		break;
	case 1:
		put_int( buf, qq->number );
		put_word( buf, 0x0007 );
		put_word( buf, 0x0008 );
		break;
	default:
		DBG("unknown type.");
	}
	post_packet( qq, p, SESSION_KEY );
}

void prot_user_get_notice_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar result, cmd;
	result = get_byte( buf );
	cmd = get_byte( buf ); 
	switch( cmd ){
	case 0x01:
		break;
	case 0x07:
		{
			ushort len = get_word( buf );
			char* str;
			NEW( str, len+1 );
			if( !str )
				return;
			get_data( buf, (uchar*)str, len );
			str[len] = 0;
			DBG("notice: %s", str );
			DEL( str );
		}
		break;
	}
}

void prot_user_check_ip( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_CHECK_IP );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 2 );
	put_byte( buf, 2 );
	put_byte( buf, 0 );
	put_int( buf, 0xD4020202 );
	put_int( buf, qq->last_login_time );
	put_byte( buf, 8 );
	put_byte( buf, 3 );
	post_packet( qq, p, SESSION_KEY );
}

void prot_user_check_ip_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	ushort len;
	char* str, *t;
	NEW( str, buf->len );
	if( !str )	return;
	t = str;
	if( get_byte( buf ) != 2 ){
		DBG("reply != 2" );
		return ;
	}
	if( buf->pos == buf->len )
		return;
	get_byte( buf );	//01
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_byte( buf );	//02
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_byte( buf );	//03
	buf->pos += 9;
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_byte( buf );	//00
	get_word( buf );	//00 11
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_word( buf );	//20 00
	get_byte( buf );	//0d
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_word( buf );	//00 00
	get_byte( buf );	//21
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_word( buf );	//30 00
	get_byte( buf );	//0d
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_word( buf );	//00 00
	get_byte( buf );	//31
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	get_byte( buf );	//40
	len = get_word( buf );
	get_data( buf, (uchar*)t, len );
	t += len;
	*t = 0;
//	DBG("str: %s", str );
	DEL( str );
}


void prot_user_get_level( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GET_LEVEL );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x88 );
	put_int( buf, qq->number );
	put_byte( buf, 0x00 );
	post_packet( qq, p, SESSION_KEY );
}

void prot_user_get_level_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd;
	cmd = get_byte( buf );
	get_int( buf );	//self number
//	DBG("cmd=%d", cmd );
	switch( cmd ){
	case 0x88:
		get_int( buf );	//00000003 unknown
		qq->level = get_word( buf ); //level
		qq->active_days = get_word( buf ); //active days
		get_word( buf ); //unknown
		qq->upgrade_days = get_word( buf ); //upgrade days
		DBG("level: %d  active_days: %d  upgrade_days: %d", qq->level, 
			qq->active_days, qq->upgrade_days );
		char event[32];
		sprintf( event, "level^$%d^$%d^$%d", qq->level, 
			qq->active_days, qq->upgrade_days );
		qqclient_put_event( qq, event );
		break;
	default:
		DBG("unknown cmd: 0x%x", cmd );
		break;
	}
	
}

void prot_user_request_token( struct qqclient* qq, uint number, uchar operation, ushort type, uint code )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_REQUEST_TOKEN );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	qq->data.operation = operation;
	if( code ){	//输入验证码
		put_byte( buf, 2 );	//sub cmd
		put_word( buf, type );	//
		put_int( buf, number );
		put_word( buf, 4 );
		put_int( buf, htonl(code) );
		put_word( buf, strlen(qq->data.qqsession));
		put_data( buf, (uchar*)qq->data.qqsession, strlen(qq->data.qqsession));
	}else{
		put_byte( buf, 1 );	//sub cmd
		put_word( buf, type );	//
		put_int( buf, number );
		qq->data.operating_number = number ;
	}
	post_packet( qq, p, SESSION_KEY );
}

void prot_user_request_token_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	get_word( buf );	//0006
	uchar verify = get_byte( buf );
	if( verify ){
		char *url, *data, *session;
		int datalen = KB(4);
		DBG("need verifying...");
		if( buf->pos == buf->len )	{
			puts("Verifying code is incorrect!");
			return;	//verify code wrong.
		}
		int len, ret;
		len = get_word( buf );
		if( len >= 128 ){
			DBG("url is too long.");	
			return;
		}
		NEW( data, datalen );
		NEW( url, 128 );
		NEW( session, 128 );
		get_data( buf, (uchar*)url, len );
		ret = http_request( &qq->http_sock, url, session, data, &datalen );
		if( ret == 0 ){
			char path[PATH_LEN];
			sprintf( path, "%s/%u.jpg", qq->verify_dir, qq->number );
			FILE *fp;
			fp = fopen( path, "wb" );
			DBG("got png at %s", path );
			if( fp ){
				fwrite( data, datalen, 1, fp );
				fclose( fp );
			}
			strncpy( qq->data.qqsession, session, 127 );
			qqclient_set_process( qq, P_VERIFYING );
			puts("You need to input the verifying code.");
		}else{
			DBG("http_request failed. ret=%d", ret );
		}
		DEL( data );
		DEL( url );
		DEL( session );
	}else{
		get_token( buf, &qq->data.user_token );
		qq->data.user_token_time = time(NULL);
		DBG("got token");
		qqbuddy *b = buddy_get( qq, qq->data.operating_number, 0 );
		if( b ){
			switch( qq->data.operation ){
			case OP_ADDBUDDY:
				if( b->verify_flag == VF_VERIFY ){
					prot_buddy_verify_addbuddy( qq, 02, qq->data.operating_number );
				}else if( b->verify_flag == VF_OK ){
					prot_buddy_verify_addbuddy( qq, 00, qq->data.operating_number );
				}
				break;
			case OP_DELBUDDY:
				prot_buddy_del_buddy( qq, qq->data.operating_number );
				break;
			}
		}
	}
	cmd = 0;
}


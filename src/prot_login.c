/*
 *  prot_login.c
 *
 *  QQ Protocol. Part Login
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  Touching server, getting login token, getting password token, getting loginkey.
 *  sending login information, logging out.
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
#include "crc32.h"
#include "protocol.h"
#include "group.h"
#include "buddy.h"
#include "qun.h"
#include "qqconn.h"

#define RANDOM
#ifdef RANDOM
#endif

#define USE_RANDKEY


static int rand2()
{
	return (rand()<<16)+rand();
}
static void randkey(uchar* key)
{
	int i;
	for( i=0; i<16; i++ )
		key[i] = rand2();
}

static void restore_version_data( struct qqclient* qq )
{
	static uchar locale[] = QQ09_LOCALE; 
	static uchar verspec[] = QQ09_VERSION_SPEC; 
	static uchar exehash[] = QQ09_EXE_HASH;
	memcpy( qq->data.locale, locale, sizeof(locale) );
	memcpy( qq->data.version_spec, verspec, 
		sizeof(verspec) );
	memcpy( qq->data.exe_hash, exehash, 
		sizeof(exehash) );
}

void prot_login_touch( struct qqclient* qq )
{
	static uchar zeros[15]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	prot_login_touch_with_info( qq, zeros, sizeof(zeros) );
}

void prot_login_touch_with_info( struct qqclient* qq, uchar* server_data, uchar len )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_TOUCH );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	restore_version_data( qq );
	randkey( p->key );
	memcpy( qq->data.server_data, server_data, MIN(len,sizeof(qq->data.server_data)) );
	put_word( buf, 0x0001 );
	put_data( buf, qq->data.locale, sizeof(qq->data.locale) );
	put_data( buf, qq->data.version_spec, sizeof(qq->data.version_spec) );
	put_data( buf, server_data, sizeof(qq->data.server_data) );
	post_packet( qq, p, RANDOM_KEY );
}

void prot_login_touch_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar result = get_byte( buf ), test_result;
	if( result == 0x00 ){
		qq->server_time = get_int( buf );
		qq->server_ip = get_int( buf );
		buf->pos += 8;	//zeros
		get_token( buf, &qq->data.login_token );
		test_result = get_byte( buf );
		if( test_result == 0 ){
			prot_login_request( qq, NULL, 0, 0 );
		}else{
			qq->data.server_info.w_redirect_count = 1;
			qq->data.server_info.c_redirect_count = get_byte( buf );
			qq->data.server_info.conn_isp_id = get_int( buf );
			qq->data.server_info.server_reverse = get_int( buf );
			qq->data.server_info.conn_ip = 0; //qq->server_ip;
			qq->server_ip = get_int( buf );
			struct in_addr addr;
			addr.s_addr = htonl( qq->server_ip );
			DBG("redirecting to %s", inet_ntoa( addr ) );
			if( qq->network == PROXY_HTTP ){
				qqconn_connect( qq );
				qqconn_establish( qq );
			}else{
				qqconn_connect( qq );
				prot_login_touch_with_info( qq, qq->data.server_data, sizeof(qq->data.server_data) );
			}
		}
	}else{
		DBG("result!=00: %d", result );
	}
}

void prot_login_request( struct qqclient* qq, token* tok, const char* code, char png_data )
{
	
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_LOGIN_REQUEST );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_word( buf, 0x0001 );
	put_data( buf, qq->data.locale, sizeof(qq->data.locale) );
	put_data( buf, qq->data.version_spec, sizeof(qq->data.version_spec) );
	//
	put_word( buf, qq->data.login_token.len );
	put_data( buf, qq->data.login_token.data, qq->data.login_token.len );
	if( code )
		put_byte( buf, 4 );
	else
		put_byte( buf, 3 );
	put_byte( buf, 0 );
	put_byte( buf, 5 );
	put_int( buf, 0 );
	put_byte( buf, png_data );
	if( code && tok ){
		put_byte( buf, 4 );
		put_data(buf,(uchar*)code, 4);
		//answer token
		put_word( buf, tok->len );
		put_data( buf, tok->data, tok->len );
	}else if( png_data && tok ){
		//png token
		put_word( buf, tok->len );
		put_data( buf, tok->data, tok->len );
	}else{
		put_byte( buf, 0 );
		put_byte( buf, 0 );
	}
	post_packet( qq, p, RANDOM_KEY );
}

void prot_login_request_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	token answer_token;
	token png_token;
	uchar next = 0;
	uchar result = get_byte( buf );	//03: ok   04: need verifying
	get_byte( buf ); //00
	get_byte( buf ); //05
	uchar png_data = get_byte( buf );
	get_int( buf );	//需要验证码时为00 00 01 23，不需要时为全0
	get_token( buf, &answer_token );
	if( png_data == 1 ){	//there's data for the png picture
		int len;
		len = get_word( buf );
		uchar* data;
		NEW( data, len );
		if( !data )
			return;
		get_data( buf, data, len );
		get_byte( buf ); //00
		next = get_byte( buf );
		char path[PATH_LEN];
		sprintf( path, "%s/%u.png", qq->verify_dir, qq->number );
		FILE *fp;
		if( next ){
			fp = fopen( path, "wb" );
		}else{
			fp = fopen( path, "ab" );
			DBG("got png at %s", path );
		}
		if( fp ){
			fwrite( data, len, 1, fp );
			fclose( fp );
		}
		DEL( data );
		get_token( buf, &png_token );
	}
	//parse the data we got
	if( png_data ){
		if( next ){
			prot_login_request( qq, &png_token, 0, 1 );
		}else{
			qq->data.verify_token = answer_token;
			qqclient_set_process( qq, P_VERIFYING );
		}
	}else{
		DBG("process verify password");
		qq->data.token_c = answer_token;
#ifdef LOGIN_WAIT
		qqclient_set_process( qq, P_WAITING );
#else
		prot_login_verify( qq );
#endif
	}
	result = 0;
}


void prot_login_verify( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_LOGIN_VERIFY );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	bytebuffer *verify_data;
	NEW( verify_data, sizeof(bytebuffer) );
	if( !verify_data ) {
		packetmgr_del_packet( &qq->packetmgr, p );
		return;
	}
	verify_data->size = PACKET_SIZE;
	put_int( verify_data, rand2() );	//random??
	put_word( verify_data, 0x0001 );
	put_int( verify_data, qq->number );
	put_data( verify_data, qq->data.version_spec, sizeof(qq->data.version_spec) );
	put_byte( verify_data, 00 );
	put_word( verify_data, 0x0000 );	//0x0000 什么来的？
	put_data( verify_data, qq->md5_pass1, 16 );
	put_int( verify_data, qq->server_time );
	verify_data->pos += 13;
	put_int( verify_data, qq->server_ip );
	put_int( verify_data, 0 );
	put_int( verify_data, 0 );
	put_word( verify_data, 0x0010 );
	put_data( verify_data, qq->data.verify_key1, 0x10 );
	put_data( verify_data, qq->data.verify_key2, 0x10 );
	//
	put_word( buf, 0x00DE );	//sub cmd??
	put_word( buf, 0x0001 );
	put_data( buf, qq->data.locale, sizeof(qq->data.locale) );
	put_data( buf, qq->data.version_spec, sizeof(qq->data.version_spec) );
	put_word( buf, qq->data.token_c.len );
	put_data( buf, qq->data.token_c.data, qq->data.token_c.len );
	if( verify_data->pos != 104 ){ DBG("wrong pos!!!");	}
	
	int out_len = 120;
	uchar encrypted[120+10];
	qqencrypt( verify_data->data, verify_data->pos, qq->md5_pass_qq, encrypted, &out_len );
	put_word( buf, out_len );
	put_data( buf, encrypted, out_len );
	
	put_word( buf, 0x0014 );
	static uchar unknown5[] = {0xBB,0x7E,0xAF,0x56,0x40,0x09,0xE8,0xAA,
		0xC6,0x23,0x02,0x78,0x27,0x12,0x4A,0xD4,0xAB,0x3B,0x4E,0x30 };
#ifdef USE_RANDKEY
	randkey( unknown5 );
#endif
	put_data( buf, unknown5, sizeof(unknown5) );
	put_word( buf, 0x0177 );
	put_byte( buf, 0x2E );	//length of the following info
	static uchar unknown6[] = {0x92,0xA7,0xAC,0x76,0xD8,
		0x13,0xCD,0x12,0x26,0xCD,0x31,0x82,0x55,0x90,0x8C,0x4B };
	static uchar unknown7[] = {0xCB,0x8D,0xA4,0xE2,0x61,0xC2,
		0xDD,0x27,0x39,0xEC,0x8A,0xCA,0xA6,0x98,0xF8,0x9B };
#ifdef USE_RANDKEY
	randkey( unknown6 );
	randkey( unknown7 );
#endif
	put_byte( buf, 0x01 );
#ifdef USE_RANDKEY
	put_int( buf, rand2()  );
#else
	put_int( buf, 0x71897D4C  );
#endif
	put_word( buf, sizeof(unknown6) );
	put_data( buf, unknown6, sizeof(unknown6) );
	put_byte( buf, 0x02 );
#ifdef USE_RANDKEY
	put_int( buf, rand2()  );
#else
	put_int( buf, 0xF8854F31  );
#endif
	put_word( buf, sizeof(unknown7) );
	put_data( buf, unknown7, sizeof(unknown7) );
	buf->pos += 328;	//all zeros
	
	DEL( verify_data );
	post_packet( qq, p, RANDOM_KEY );
}

void prot_login_verify_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	get_word( buf );	//length or sth..
	int code = get_byte( buf );
	switch( code ){
	case 0x00:
		get_token( buf, &qq->data.login_info_token );
		qq->data.login_info_unknown1 = get_int( buf );
		qq->server_time = get_int( buf );
		get_token( buf, &qq->data.login_info_data );
		get_token( buf, &qq->data.login_info_magic );
		get_data( buf, qq->data.login_info_key1, 16 );
		get_word( buf );	//00 00
		get_data( buf, qq->data.login_info_key3, 16 );
		get_word( buf );	//00 00
		//success
		prot_login_get_info( qq );
		return;
	case 0x33:
	case 0x51:	//
		qqclient_set_process( qq, P_DENIED );
		DBG("Denied.");
		break;
	case 0xBF:
		DBG("No this number.");
	case 0x34:
		DBG("Wrong password.");
		qqclient_set_process( qq, P_WRONGPASS );
		break;
	default:
		hex_dump( buf->data, buf->len );
		break;
	}
	buf->pos = 11;
	uchar len = get_word( buf );
	char msg[256];
	get_data( buf, (uchar*)msg, len );
	msg[len] = 0;
	DBG( msg );
	//failed
}


void prot_login_get_info( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_LOGIN_GET_INFO );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_word( buf, 0x010E );	//length or sth..
	put_byte( buf, 0x00 );
	put_word( buf, 0x0101 );
	put_data( buf, qq->data.locale, sizeof(qq->data.locale) );
	put_data( buf, qq->data.version_spec, sizeof(qq->data.version_spec) );
	put_word( buf, qq->data.token_c.len );
	put_data( buf, qq->data.token_c.data, qq->data.token_c.len );
	put_word( buf, qq->data.login_info_token.len );
	put_data( buf, qq->data.login_info_token.data, qq->data.login_info_token.len );
	put_int( buf, qq->data.login_info_unknown1 );
	put_int( buf, qq->server_time );
	put_word( buf, qq->data.login_info_data.len );
	put_data( buf, qq->data.login_info_data.data, qq->data.login_info_data.len );
	put_word( buf, 0x0000 );
	put_byte( buf, 0x01 );
	put_int( buf, 0x00000000 );
	memcpy( p->key, qq->data.login_info_key1, sizeof(qq->data.login_info_key1) );
	post_packet( qq, p, RANDOM_KEY );
}

void prot_login_get_info_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	get_word( buf );	//01 66 length or sth...
	get_word( buf );	//01 00
	get_data( buf, qq->data.login_info_key2, 16 );
	buf->pos += 8;	//00 00 00 01 00 00 00 64 
	qq->data.login_info_unknown2 = get_int( buf );  //00 C8 00 02
	qq->server_time = get_int( buf );
	qq->client_ip = get_int( buf );
	get_int( buf );	//00000000
	get_int( buf );	//00000000
	get_token( buf, &qq->data.login_info_large );
	/*
	get_int( buf );	//????
	uchar len = get_byte( buf );
	get_data( buf, (void*)qq->self->nickname, len );
	qq->self->nickname[len] = 0;
	DBG("Hello, %s", qq->self->nickname );
//	prot_login_a4( qq );
	*/
	prot_login_send_info( qq );
//	prot_login_get_list( qq, 0 );
}

void prot_login_a4( struct qqclient* qq )
{
	static uchar unknown[] = {0x10,0x03,0xC8,0xEC,0xC8,0x96,
	0x8B,0xF2,0xB3,0x6B,0x4D,0x0C,0x5C,0xE0,0x6A,0x51,0xCE };
 
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_LOGIN_A4 );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_word( buf, 0x0101 );
	put_word( buf, 0x0000 );
	put_byte( buf, qq->data.login_info_token.len );
	put_data( buf, qq->data.login_info_token.data, qq->data.login_info_token.len );
	put_data( buf, unknown, sizeof(unknown) );
	memcpy( p->key, qq->data.login_info_key1, sizeof(qq->data.login_info_key1) );
	post_packet( qq, p, RANDOM_KEY );
}


void prot_login_a4_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	get_word( buf );
	//2009-1-26 19:04 Huang Guan: for getting the whole list
	qq->data.login_list_count = 0;
	prot_login_get_list( qq, 0 );
}


void prot_login_send_info( struct qqclient* qq )
{
	static uchar unknown5[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
		0x00,0x00,0x00 };
	static uchar unknown6[] = {0x92,0xA7,0xAC,0x76,0xD8,
		0x13,0xCD,0x12,0x26,0xCD,0x31,0x82,0x55,0x90,0x8C,0x4B };
	static uchar unknown7[] = {0xCB,0x8D,0xA4,0xE2,0x61,0xC2,
		0xDD,0x27,0x39,0xEC,0x8A,0xCA,0xA6,0x98,0xF8,0x9B };
		
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_LOGIN_SEND_INFO );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	//prepare sth.
#ifdef USE_RANDKEY
	randkey( unknown6 );
	randkey( unknown7 );
#endif
	
	put_word( buf, 0x0001 );
	put_data( buf, qq->data.version_spec, sizeof(qq->data.version_spec) );
	put_int( buf, qq->data.login_info_unknown2 );
	put_int( buf, qq->server_time );
	put_int( buf, qq->client_ip );
	put_int( buf, 00000000 );
	put_int( buf, 00000000 );
	put_word( buf, qq->data.login_info_large.len );
	put_data( buf, qq->data.login_info_large.data, qq->data.login_info_large.len );
	buf->pos += 35;
	put_data( buf, qq->data.exe_hash, sizeof(qq->data.exe_hash) );
	put_byte( buf, rand2() );	//unknown important byte
	put_byte( buf, qq->mode );
	put_data( buf, unknown5, sizeof(unknown5) );
	put_data( buf, qq->data.server_data, sizeof(qq->data.server_data) );
	put_data( buf, qq->data.locale, sizeof(qq->data.locale) );
	buf->pos += 16; //16 zeros
	put_word( buf, qq->data.token_c.len );
	put_data( buf, qq->data.token_c.data, qq->data.token_c.len );
	put_int( buf, 0x00000008 );
	put_int( buf, 0x00000000 );
	put_int( buf, 0x08041000 );
	put_byte( buf, 0x01 );
	put_byte( buf, 0x40 );	//length of the following
	put_byte( buf, 0x01 );
#ifdef USE_RANDKEY
	put_int( buf, rand2()  );
#else
	put_int( buf, 0x71897D4C  );
#endif
	put_word( buf, sizeof(unknown6) );
	put_data( buf, unknown6, sizeof(unknown6) );
	put_data( buf, unknown5, sizeof(unknown5) );
	put_data( buf, qq->data.server_data, sizeof(qq->data.server_data) );
//	buf->pos += 15;
	put_byte( buf, 0x02 );
#ifdef USE_RANDKEY
	put_int( buf, rand2()  );
#else
	put_int( buf, 0xF8854F31  );
#endif
	put_word( buf, sizeof(unknown7) );
	put_data( buf, unknown7, sizeof(unknown7) );
	buf->pos += 251;	//all zeros
	memcpy( p->key, qq->data.login_info_key1, sizeof(qq->data.login_info_key1) );
	post_packet( qq, p, RANDOM_KEY );
}

void prot_login_send_info_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
//	hex_dump( buf->data, buf->len );
	uchar result = get_byte( buf );
	if( result != 0 )
	{
		DBG("login result = %d", result );
		qqclient_set_process( qq, P_ERROR );
		return;
	}
	get_data( buf, qq->data.session_key, sizeof(qq->data.session_key) );
	DBG("session key: " );
	hex_dump( qq->data.session_key, 16 );
	if( qq->number != get_int( buf ) ){
		DBG("qq->number is wrong?");
	}
	qq->client_ip = get_int( buf );
	qq->client_port = get_word( buf );
//	qq->local_ip = get_int( buf );
//	qq->local_port = get_word( buf );
	qq->login_time = get_int( buf );
	get_int( buf ); // 0x00000000
	get_byte( buf );	//03
	get_byte( buf );	//mode
//	buf->pos += 96;
//	qq->last_login_time = get_int( buf );
	//prepare IM key
	uchar data[20];
	*(uint*)data = htonl( qq->number );
	memcpy( data+4, qq->data.session_key, 16 );
	//md5
	md5_state_t mst;
	md5_init( &mst );
	md5_append( &mst, (md5_byte_t*)data, 20 );
	md5_finish( &mst, (md5_byte_t*)qq->data.im_key );
	//
	time_t t;
	t = CN_TIME( qq->login_time );
	DBG("login time: %s", ctime( &t ) );
	qqclient_set_process( qq, P_LOGIN );

	prot_login_get_list( qq, 1 );
}


void prot_login_get_list( struct qqclient* qq, ushort pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_LOGIN_GET_LIST );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 1 );
	put_int( buf, 0 );
	put_int( buf, 0 );
	put_word( buf, pos ); //index
	post_packet( qq, p, SESSION_KEY );
}

void prot_login_get_list_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	ushort current_page, total_pages, count;
	uchar subcmd = get_byte( buf );	//01
	int i;
	uchar result = get_byte( buf ); //00
	if( result == 00 ){
		get_int( buf );
		get_int( buf ); //00 00 07 E6
		get_int( buf ); //4C A2 20 AE  Last Update Time
		total_pages = get_word( buf );
		current_page = get_word( buf );
		count = get_word( buf );
		for( i=0; i<count; i++ ){
			uint number = get_int( buf );
			uchar type = get_byte( buf );
			uchar gid = get_byte( buf );
			if( type == 0x04 )	//if it is a qun
			{
#ifndef NO_QUN_INFO
				DBG("got qun: %d", number );
				qun_get( qq, number, 1 );
#endif
			}else if( type == 0x01 ){
#ifndef NO_BUDDY_INFO
				qqbuddy* b = buddy_get( qq, number, 1 );
				DBG("got buddy: %d", number );
				if( b )
					b->gid = gid / 4;
#endif
			}
			number = type = gid = 0;
		}
		if( current_page < total_pages ){
			prot_login_get_list( qq, 1 + current_page );
			return;
		}
	}
	subcmd = 0;
	prot_login_e9( qq );
}

void prot_login_e9( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_E9 );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_word( buf, 0x0101 );
	post_packet( qq, p, SESSION_KEY );
}


void prot_login_e9_reply( struct qqclient* qq, qqpacket* p )
{
//	bytebuffer *buf = p->buf;
	prot_login_ea( qq );
}

void prot_login_ea( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_EA );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x01 );
	post_packet( qq, p, SESSION_KEY );
}


void prot_login_ea_reply( struct qqclient* qq, qqpacket* p )
{
//	bytebuffer *buf = p->buf;
	prot_login_ed( qq );
}


void prot_login_ed( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_ED );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x01 );
	post_packet( qq, p, SESSION_KEY );
}


void prot_login_ed_reply( struct qqclient* qq, qqpacket* p )
{
//	bytebuffer *buf = p->buf;
	prot_login_ec( qq );
}

void prot_login_ec( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_EC );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x01 );
	put_word( buf, 0x000A );
	post_packet( qq, p, SESSION_KEY );
}


void prot_login_ec_reply( struct qqclient* qq, qqpacket* p )
{
//	bytebuffer *buf = p->buf;
	//get information
//#ifndef NO_GROUP_INFO
	group_update_list( qq );
	prot_user_get_level( qq );
	prot_user_change_status( qq );
//#endif
//#ifndef NO_BUDDY_INFO
	buddy_update_list( qq );
//#endif
//#ifndef NO_QUN_INFO
	qun_update_all( qq );
//#endif
	qq->online_clock = 0;
}

void prot_login_logout( struct qqclient* qq )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_LOGOUT );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	buf->pos += 16;	//zeros
	p->need_ack = 0;
	post_packet( qq, p, SESSION_KEY );
}

/*
 *  prot_qun.c
 *
 *  QQ Protocol. Part Qun
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  
 *
 */

#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "qqclient.h"
#include "memory.h"
#include "debug.h"
#include "qqpacket.h"
#include "packetmgr.h"
#include "protocol.h"
#include "qun.h"
#include "utf8.h"

void prot_qun_get_info( struct qqclient* qq, uint number, uint pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x72 );	//command?
	put_int( buf, number );	//
	put_int( buf, pos );	//
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_send_msg( struct qqclient* qq, uint number, char* msg_content )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	ushort len = strlen( msg_content );
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x2A );
	put_int( buf, number );
	bytebuffer* content_buf;
	NEW( content_buf, sizeof(bytebuffer) );
	if( !content_buf ) {
		packetmgr_del_packet( &qq->packetmgr, p );
		return;
	}
	content_buf->size = PACKET_SIZE;
	
	put_word( content_buf, 0x0001 );	//text type
	put_byte( content_buf, 0x01 );		//slice_count
	put_byte( content_buf, 0x00 );		//slice_no
	put_word( content_buf, 0 );		//id??
	put_int( content_buf, 0 );		//zeros

	put_int( content_buf, 0x4D534700 ); //"MSG"
	put_int( content_buf, 0x00000000 );
	put_int( content_buf, p->time_create );
	put_int( content_buf, rand() );
	put_int( content_buf, 0x00000000 );
	put_int( content_buf, 0x09008600 );
	char font_name[] = "宋体";	//must be in UTF8
	put_word( content_buf, strlen(font_name) );
	put_data( content_buf, (void*)font_name, strlen( font_name) );
	put_word( content_buf, 0x0000 );
	put_byte( content_buf, 0x01 );
	put_word( content_buf, len+3 );
	put_byte( content_buf, 1 );			//unknown, keep 1
	put_word( content_buf, len );
	put_data( content_buf, (uchar*)msg_content, len );
	
	put_word( buf, content_buf->pos );
	put_data( buf, content_buf->data, content_buf->pos );
	DEL( content_buf );
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_get_memberinfo( struct qqclient* qq, uint number, uint* numbers, int count )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x0C );	//command?
	put_int( buf, number );	//
	int i;
	if( count > 30 ) count = 30;	//TXQQ一次获取30个。
	for( i=0; i<count; i++ ){
		put_int( buf, numbers[i] );	//
	}
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_get_online( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x0B );	//command?
	put_int( buf, number );	//
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_get_membername( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x0F );	//command?
	put_int( buf, number );	//
	put_int( buf, 0x0 );	//?? which is position??
	put_int( buf, 0x0 );	//??
	post_packet( qq, p, SESSION_KEY );
}

static void parse_quninfo( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	uint last_number;
	uchar more, status;
	bytebuffer *buf = p->buf;
	q->ext_number = get_int( buf );
	get_word( buf );	//00 00
	get_byte( buf );	//00
	status = get_byte( buf );	//03 or 02
	if( status == 3 ){
		q->type = get_byte( buf );
		get_int( buf );	//unknown
	//	get_int( buf );	//(???)unknown in 1205
		q->owner = get_int( buf );
		q->auth_type = get_byte( buf );
		buf->pos += 6;
		q->category = get_int( buf );
		q->max_member = get_word( buf );
		buf->pos += 9;
		//name
		uchar len = get_byte( buf );
		len = MIN( NICKNAME_LEN-1, len );
		get_data( buf,  (uchar*)q->name, len );
		q->name[len] = 0;
	//	DBG("qun: %s", q->name );
		get_byte( buf );
		get_byte( buf );	//separator
		//ann
		len = get_byte( buf );
		
		get_data( buf,  (uchar*)q->ann, len );
		q->ann[len] = 0;
		//intro
		len = get_byte( buf );
		get_data( buf,  (uchar*)q->intro, len );
		q->intro[len] = 0;
		//token data
		get_token( buf, &q->token_cmd );
	}
	last_number = get_int( buf );	//member last came in
	more = get_byte( buf );	//more member data
	while( buf->pos < buf->len ){
		uint n = get_int( buf );
		uchar org = get_byte( buf );
		uchar role = get_byte( buf );
		qunmember* m = qun_member_get( qq, q, n, 1 );
		if( m==NULL ){
			DBG("m==NULL");
			break;
		}
		m->org = org;
		m->role = role;
	}
	if( more ){
		prot_qun_get_info( qq, q->number, last_number );
	}else{
		qun_update_memberinfo( qq, q );
		qun_set_members_off( qq, q );
		qun_update_online( qq, q );
	}
}

static void parse_memberinfo( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	bytebuffer *buf = p->buf;
	while( buf->pos < buf->len ){
		uint number = get_int( buf );
		qunmember* m = qun_member_get( qq, q, number, 0 );
		if( !m ){
			DBG("m==NULL  number: %d", number);
			break;
		}
		m->face = get_word( buf );
		m->age = get_byte( buf );
		m->sex = get_byte( buf );
		uchar name_len = get_byte( buf );
		name_len = MIN( NICKNAME_LEN-1, name_len );
		get_data( buf,  (uchar*)m->nickname, name_len );
		m->nickname[name_len] = 0;
		//TX技术改革不彻底，还保留使用GB码 2009-1-25 11:02
		gb_to_utf8( m->nickname, m->nickname, NICKNAME_LEN-1 );
		get_word( buf );	//00 00
		m->qqshow = get_byte( buf );
		m->flag = get_byte( buf );
	}
}

static void parse_online( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	bytebuffer *buf = p->buf;
	get_byte( buf );
	//set all off
	qun_set_members_off( qq, q );
	while( buf->pos < buf->len ){
		uint number = get_int( buf );
		qunmember* m = qun_member_get( qq, q, number, 1 );
		if( m )
			m->status = QQ_ONLINE;
	}
	qun_put_single_event( qq, q );
}

static void parse_membername( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	bytebuffer *buf = p->buf;
	uint pos;
	pos = get_int( buf );
	get_int( buf );	//00000000
	while( buf->pos < buf->len ){
		uint number = get_int( buf );
		qunmember* m = qun_member_get( qq, q, number, 0 );
		if( !m ){
			DBG("m==NULL");
			break;
		}
		uchar name_len = get_byte( buf );
		name_len = MIN( NICKNAME_LEN-1, name_len );
		get_data( buf,  (uchar*)m->nickname, name_len );
		m->nickname[name_len] = 0;
	}
}

void prot_qun_cmd_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	uchar result = get_byte( buf );
	if( result != 0 ){
		DBG("result = %d", result );
		return ;
	}
	uint number = get_int( buf );
	qqqun* q = qun_get( qq, number, 0 );
	if( !q ){
		DBG("q==null");
		return;
	}
	switch( cmd ){
		case 0x2A:	//send msg;
			break;
		case 0x72:
			parse_quninfo( qq, p, q );
			break;
		case 0x0C:
			parse_memberinfo( qq, p, q );
			break;
		case 0x0B:
			parse_online( qq, p, q );
			break;
		case 0x0F:
			parse_membername( qq, p, q );
			break;
		default:
			DBG("unknown cmd = %x", cmd );
	}

}


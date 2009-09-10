/*
 *  prot_im.c
 *
 *  QQ Protocol. Part Internet Message
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  sending/receiving buddy or qun messages
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
#include "buddy.h"
#include "protocol.h"
#include "utf8.h"
#include "util.h"

// we divide the message into pieces if it's too long
void prot_im_send_msg( struct qqclient* qq, uint to, char* msg )
{
	const int max_length = 700;	//TX made it.
	int pos=0, end_pos, slice_count, i;
	int len = strlen( msg );
	slice_count = len / max_length + 1;	//in fact this not reliable.
	ushort msg_id = (ushort)rand();
	for( i=0; i<slice_count; i++ ){
		end_pos = get_splitable_pos( msg, pos+MIN( len-pos, max_length ) );	
		DBG("%u send (%d,%d) %d/%d", qq->number, pos, end_pos, i, slice_count );
		//msg[pos] might be 0
		prot_im_send_msg_ex( qq, to, &msg[pos], end_pos-pos, msg_id, slice_count, i );
		pos = end_pos;
	}
}

void prot_im_send_msg_ex( struct qqclient* qq, uint to, char* msg, int len,
	ushort msg_id, uchar slice_count, uchar which_piece )
{
//	DBG("str: %s  len: %d", msg, len );
	qqpacket* p;
	if( !len ) return;
	p = packetmgr_new_send( qq, QQ_CMD_SEND_IM );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_int( buf, qq->number );
	put_int( buf, to );
	//00 00 00 08 00 01 00 04 00 00 00 00  09SP1 changes
	put_int( buf, 0x00000008 );
	put_int( buf, 0x00010004 );
	put_int( buf, 0x00000000 );
	put_word( buf, qq->version );
	put_int( buf, qq->number );
	put_int( buf, to );
	put_data( buf, qq->data.im_key, 16 );
	put_word( buf, QQ_NORMAL_IM_TEXT );	//message type
	put_word( buf, p->seqno );
	put_int( buf, p->time_create );
	put_word( buf, qq->self->face );	//my face
	put_int( buf, 1 );	//has font attribute
	put_byte( buf, slice_count );	//slice_count
	put_byte( buf, which_piece );	//slice_no
	put_word( buf, msg_id );	//msg_id??
	put_byte( buf, QQ_IM_TEXT );	//auto_reply
	put_int( buf, 0x4D534700 ); //"MSG"
	put_int( buf, 0x00000000 );
	put_int( buf, p->time_create );
	put_int( buf, (msg_id<<16)|msg_id );	//maybe a random interger
	put_int( buf, 0x00000000 );
	put_int( buf, 0x09008600 );
	char font_name[] = "宋体";	//must be UTF8
	put_word( buf, strlen(font_name) );
	put_data( buf, (void*)font_name, strlen( font_name) );
	put_word( buf, 0x0000 );
	put_byte( buf, 0x01 );
	put_word( buf, len+3 );
	put_byte( buf, 1 );
	put_word( buf, len );
//	put_word( buf, p->seqno );
	put_data( buf, (uchar*)msg, len );
	post_packet( qq, p, SESSION_KEY );
}

void prot_im_ack_recv( struct qqclient* qq, qqpacket* pre )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_RECV_IM );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	p->seqno = pre->seqno;
	put_data( buf, pre->buf->data, 16 );
	p->need_ack = 0;
	post_packet( qq, p, SESSION_KEY );
}

static void parse_message_09( qqpacket* p, qqmessage* msg, char* tmp, int outlen )
{
	bytebuffer *buf = p->buf;
	int i = 0;
	ushort len;
	buf->pos += 8; //'M' 'S' 'G' 00 00 00 00 00
	msg->msg_time = get_int( buf );	//send time
	buf->pos += 12;
	len = get_word( buf );	//pass font name
	buf->pos += len;
	get_word( buf );	//00 00 2009-2-7 Huang Guan, updated
	while( buf->pos < buf->len ){
		uchar ch;
		uchar type = get_byte(buf);
		len = get_word(buf);
		get_byte(buf);	//is 01 if text or face, 02 if image
		ushort len_str;
		switch( type ){
		case 01:	//pure text
			len_str = get_word( buf );
			len_str = MIN(len_str, outlen-i);
			get_data( buf, (void*)&tmp[i], len_str );
			i += len_str;
			break;
		case 02:	//face
			len_str = get_word( buf );
			buf->pos += len_str;	//
			get_byte( buf );	//FF
			len_str = get_word( buf );
			if( len_str == 2 ){
				get_byte( buf );
				if( outlen-i > 15 )
					i += sprintf( &tmp[i], "[face:%d]", get_byte( buf ) );
			}else{ //unknown situation
				buf->pos += len_str;
			}
			break;
		case 03:	//image
			len_str = get_word( buf );
			buf->pos += len_str;	//
			do{
				ch = get_byte( buf );	//ff
			}while( ch!=0xff && buf->pos<buf->len );
			token tok_pic;
			get_token( buf, &tok_pic );
			if( outlen-i > 10 )
				i += sprintf( &tmp[i], "[image]" );
		}
		len = 0;	//use it, or the compiler would bark.
	}
	tmp[i] = 0;
}

static void process_buddy_im_text( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	char tmp[MSG_CONTENT_LEN];
	get_word( buf );	//session id
	msg->msg_time = get_int( buf );
	get_word( buf );	//face
	buf->pos += 4; 	//0000001
	//分片
	msg->slice_count = get_byte( buf );
	msg->slice_no = get_byte( buf );
	msg->msg_id = get_word( buf );
	msg->auto_reply = get_byte( buf );
	switch( msg->im_type ){
	case QQ_RECV_IM_BUDDY_09:
	case QQ_RECV_IM_BUDDY_09SP1:
		parse_message_09( p, msg, tmp, MSG_CONTENT_LEN );
		strcpy( msg->msg_content, tmp );
		break;
	case QQ_RECV_IM_BUDDY_0801:
		get_string( buf, tmp, MSG_CONTENT_LEN );
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN-1 );
		trans_faces( tmp, msg->msg_content, MSG_CONTENT_LEN );
		break;
	case QQ_RECV_IM_BUDDY_0802:
		buf->pos += 8;
		get_string( buf, tmp, MSG_CONTENT_LEN );
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN-1 );
		trans_faces( tmp, msg->msg_content, MSG_CONTENT_LEN );
		break;
	}
//	DBG("buddy msg from %u:", msg->from );
	if( qq->auto_reply[0]!='\0' ){ //
		prot_im_send_msg( qq, msg->from, qq->auto_reply );
	}
	buddy_msg_callback( qq, msg->from, msg->msg_time, msg->msg_content );
}

static void process_buddy_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	get_word( buf );	//version
	msg->from = get_int( buf );
	if( get_int( buf ) != qq->number ){
		DBG("nothing but this is impossible!!");
		return;
	}
	//to check if this buddy is in our list, or we add it.
	buddy_get( qq, msg->from, 1 );
	//IM key
	uchar key[16];
	get_data( buf, key, 16 );
	ushort content_type = get_word( buf );
	switch( content_type ){
	case QQ_NORMAL_IM_TEXT:
	//	DBG("QQ_NORMAL_IM_TEXT");
		process_buddy_im_text( qq, p, msg );
		break;
	case QQ_NORMAL_IM_FILE_REQUEST_TCP:
		DBG("QQ_NORMAL_IM_FILE_REQUEST_TCP");
		break;
	case QQ_NORMAL_IM_FILE_APPROVE_TCP:
		DBG("QQ_NORMAL_IM_FILE_APPROVE_TCP");
		break;
	case QQ_NORMAL_IM_FILE_REJECT_TCP:
		DBG("QQ_NORMAL_IM_FILE_REJECT_TCP");
		break;
	case QQ_NORMAL_IM_FILE_REQUEST_UDP:
		DBG("QQ_NORMAL_IM_FILE_REQUEST_UDP");
		break;
	case QQ_NORMAL_IM_FILE_APPROVE_UDP:
		DBG("QQ_NORMAL_IM_FILE_APPROVE_UDP");
		break;
	case QQ_NORMAL_IM_FILE_REJECT_UDP:
		DBG("QQ_NORMAL_IM_FILE_REJECT_UDP");
		break;
	case QQ_NORMAL_IM_FILE_NOTIFY:
		DBG("QQ_NORMAL_IM_FILE_NOTIFY");
		break;
	case QQ_NORMAL_IM_FILE_PASV:
		DBG("QQ_NORMAL_IM_FILE_PASV");
		break;
	case QQ_NORMAL_IM_FILE_CANCEL:
		DBG("QQ_NORMAL_IM_FILE_CANCEL");
		break;
	case QQ_NORMAL_IM_FILE_EX_REQUEST_UDP:
//		DBG("QQ_NORMAL_IM_FILE_EX_REQUEST_UDP");
		break;
	case QQ_NORMAL_IM_FILE_EX_REQUEST_ACCEPT:
		DBG("QQ_NORMAL_IM_FILE_EX_REQUEST_ACCEPT");
		break;
	case QQ_NORMAL_IM_FILE_EX_REQUEST_CANCEL:
		DBG("QQ_NORMAL_IM_FILE_EX_REQUEST_CANCEL");
		break;
	case QQ_NORMAL_IM_FILE_EX_NOTIFY_IP:
		DBG("QQ_NORMAL_IM_FILE_EX_NOTIFY_IP");
		break;
	default:
		DBG("UNKNOWN type: %x", content_type );
		break;
	}
}

static void process_sys_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	msg->msg_time = time(NULL);
	uchar content_type;
	content_type = get_byte( buf );
	uchar len = get_byte( buf );
	get_data( buf, (uchar*)msg->msg_content, len );
	msg->msg_content[len] = 0;
	buddy_msg_callback( qq, msg->from, msg->msg_time, msg->msg_content );
	if( strstr( msg->msg_content, "另一地点登录" ) != NULL ){
		qqclient_set_process( qq, P_BUSY );
	}else{
		qqclient_set_process( qq, P_ERROR );
	}
//	DBG("sysim(type:%x): %s", content_type, msg->msg_content );
}

static void process_qun_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	char tmp[MSG_CONTENT_LEN];
	get_int( buf );		//00 00 00 00  09SP1
	get_int( buf );		//ext_number
	get_byte( buf );	//normal qun or temp qun?
	msg->from = get_int( buf );
	if( msg->from == qq->number )
		return;
	get_word( buf );	//zero
	msg->msg_id = get_word( buf );
	msg->msg_time = get_int( buf );
	switch( msg->im_type ){
	case QQ_RECV_IM_QUN_IM_09:
		buf->pos += 16;
		parse_message_09( p, msg, tmp, MSG_CONTENT_LEN );
		strcpy( msg->msg_content, tmp );
		break;
	case QQ_RECV_IM_QUN_IM:
		buf->pos += 16;
		get_string( buf, tmp, MSG_CONTENT_LEN );
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN-1 );
		trans_faces( tmp, msg->msg_content, MSG_CONTENT_LEN );
		break;
	}
	
//	DBG("process_qun_im(number:%u): ", msg->from );
//	puts( msg->msg_content );
	qun_msg_callback( qq, msg->from, msg->qun_number, msg->msg_time, msg->msg_content );
}

static void process_qun_member_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	token tok_unknown;
	buf->pos += 14;	//00 65 01 02 00 00 00 00 00 00 00 00 00 00
	get_token( buf, &tok_unknown );	//len: 0x30
	buf->pos += 8;	//00 26 00 1c 02 00 01 00
	get_int( buf );	//unknown time 48 86 b9 e5
	get_int( buf );	//00 01 51 80
	msg->from = get_int( buf ); //from 10 4a 61 e3
	get_int( buf );	//self number
	get_int( buf );	//00 00 00 1f
	msg->msg_time = get_int( buf );	//48 86 c4 f1
	msg->qun_number = get_int( buf );	//1c aa 5d 98 
	get_int( buf );		//ext_number
	buf->pos += 15;	//00 0d 00 00 00 00 00 00 00 00 00 00 00 00 00 
	process_buddy_im( qq, p, msg );
	DBG("process_qun_member_im: qun_number: %u", msg->qun_number );
}


void prot_im_recv_msg( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uint from, from_ip;
	ushort from_port;
	ushort im_type;
	int len;
	qqmessage *msg;
	if( qq->login_finish!=1 ){	//not finished login
		DBG("Early message ... Abandoned.");
		return;
	}
	NEW( msg, sizeof( qqmessage ) );
	if( !msg )
		return;
	//
	from = get_int( buf );
	if( get_int( buf ) != qq->number ){
		DBG("qq->number is not equal to yours");
	}
	get_int( buf );	//sequence
	from_ip = get_int( buf );
	from_port = get_word( buf );
	im_type = get_word( buf );
	msg->im_type = im_type;
	switch( im_type ){
	case QQ_RECV_IM_BUDDY_0801:
		DBG("QQ_RECV_IM_BUDDY_0801");
		msg->msg_type = MT_BUDDY;
		//fixed for qq2007, webqq. 20090621 17:57
		buf->pos += 2;
		len = get_word( buf ); 
		buf->pos += len;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_BUDDY_0802:
		DBG("QQ_RECV_IM_BUDDY_0802");
		msg->msg_type = MT_BUDDY;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_BUDDY_09:
		DBG("QQ_RECV_IM_BUDDY_09");
		msg->msg_type = MT_BUDDY;
		buf->pos += 11;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_BUDDY_09SP1:
		DBG("QQ_RECV_IM_BUDDY_09SP1");
		msg->msg_type = MT_BUDDY;
		buf->pos += 2;
		len = get_word( buf ); 
		buf->pos += len;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_WRITING:
//		DBG("QQ_RECV_IM_WRITING");
		break;
	case QQ_RECV_IM_QUN_IM:
	case QQ_RECV_IM_QUN_IM_09:
		msg->msg_type = MT_QUN;
		msg->qun_number = from;
		process_qun_im( qq, p, msg );
		break;
	case QQ_RECV_IM_TO_UNKNOWN:
		DBG("QQ_RECV_IM_TO_UNKNOWN");
		break;
	case QQ_RECV_IM_NEWS:
		DBG("QQ_RECV_IM_NEWS");
		break;
	case QQ_RECV_IM_UNKNOWN_QUN_IM:
		DBG("QQ_RECV_IM_UNKNOWN_QUN_IM");
		break;
	case QQ_RECV_IM_ADD_TO_QUN:
		DBG("QQ_RECV_IM_ADD_TO_QUN");
		break;
	case QQ_RECV_IM_DEL_FROM_QUN:
		DBG("QQ_RECV_IM_DEL_FROM_QUN");
		break;
	case QQ_RECV_IM_APPLY_ADD_TO_QUN:
		DBG("QQ_RECV_IM_APPLY_ADD_TO_QUN");
		break;
	case QQ_RECV_IM_APPROVE_APPLY_ADD_TO_QUN:
		DBG("QQ_RECV_IM_APPROVE_APPLY_ADD_TO_QUN");
		break;
	case QQ_RECV_IM_REJCT_APPLY_ADD_TO_QUN:
		DBG("QQ_RECV_IM_REJCT_APPLY_ADD_TO_QUN");
		break;
	case QQ_RECV_IM_CREATE_QUN:
		DBG("QQ_RECV_IM_CREATE_QUN");
		break;
	case QQ_RECV_IM_TEMP_QUN_IM:
		DBG("QQ_RECV_IM_TEMP_QUN_IM");
	case QQ_RECV_IM_SYS_NOTIFICATION:
		msg->msg_type = MT_SYSTEM;
		get_int( buf );	//09 SP1 fixed
		process_sys_im( qq, p, msg );
		break;
	case QQ_RECV_IM_QUN_MEMBER_IM:
		msg->msg_type = MT_QUN_MEMBER;
		process_qun_member_im( qq, p, msg );
		break;
	default:
		DBG("Unknown message type : %x", im_type );
	}
	//ack recv
	prot_im_ack_recv( qq, p );
	DEL( msg );
}


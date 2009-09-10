/*
 *  prot_buddy.c
 *
 *  QQ Protocol. Part Buddy
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

#include "qqclient.h"
#include "memory.h"
#include "debug.h"
#include "qqpacket.h"
#include "packetmgr.h"
#include "protocol.h"
#include "buddy.h"
#include "config.h"

void prot_buddy_update_list( struct qqclient* qq, ushort pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GET_BUDDY_LIST );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 01 );
	put_int( buf, 00000000 );
	put_int( buf, 00000000 );
	put_byte( buf, 02 );
	put_word( buf, pos );
	put_word( buf, 0 );
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_update_list_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	buf->pos += 10;
	ushort next_pos = get_word( buf );
	buf->pos += 5;
	uint number;
	qqbuddy* b;
	while( buf->pos < buf->len-5 ){	//5 rest bytes I dont know
		number = get_int( buf );
		b = buddy_get( qq, number, 1 );
		if( b == NULL ){
			DBG("Error: failed to allocate buddy.");
			next_pos = 0xffff;
			break;
		}
		b->face = get_word( buf );
		b->age = get_byte( buf );
		b->sex = get_byte( buf );
		uchar name_len = get_byte( buf );	//name_len
		get_data( buf,  (uchar*)b->nickname, name_len );	
		b->nickname[name_len] = 0;
		buf->pos += 27;
	}
	if( next_pos != 0xffff ){
		prot_buddy_update_list( qq, next_pos );
	}else{
		DBG("buddy_count: %d", qq->buddy_list.count );
		buddy_set_all_off( qq );
#ifndef NO_BUDDY_DETAIL_INFO
		prot_buddy_update_signiture( qq, 0 );
		prot_buddy_update_account( qq, 0 );
		prot_buddy_update_alias( qq, 0 );
#endif
		buddy_put_event( qq );
	}
}


void prot_buddy_update_online( struct qqclient* qq, uint next_number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GET_BUDDY_ONLINE );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x02 );	//get buddies
	put_int( buf, next_number );
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_update_online_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar next_order = get_byte( buf );
	uint number;
	qqbuddy* b;
	uint max_number = 0;
	while( buf->pos < buf->len ){
		number = get_int( buf );
		if( number > max_number )
			max_number = number;
		b = buddy_get( qq, number, 0 );
		if( !b ){
			DBG("buddy_get(%d) failed.", number );
			next_order = 0xff;
			break;
		}
		get_byte( buf );
		b->ip = get_int( buf );
		b->port = get_word( buf );
		get_byte( buf );
		b->status = get_byte( buf );
		b->version = get_word( buf );
		get_data( buf, b->session_key, 16 );
		buf->pos += 11;
	}
	if( next_order != 0xff ){
		prot_buddy_update_online( qq, max_number );
	}else{
		buddy_put_event( qq );
		return;
	}
}


void prot_buddy_status( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uint number = get_int( buf );
	qqbuddy* b = buddy_get( qq, number, 0 );
	if( b ){
		get_byte( buf );
		b->ip = get_int( buf );
		b->port = get_word( buf );
		get_byte( buf );
		b->status = get_byte( buf );
//		DBG("status to %x", b->status );
		b->version = get_word( buf );
		get_data( buf, b->session_key, 16 );
		char event[64];
		sprintf( event, "buddystatus^$%u^$%s", number, buddy_status_string(b->status) );
		qqclient_put_event( qq, event );
	}
}


void prot_buddy_update_signiture( struct qqclient* qq, uint pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GET_BUDDY_SIGN );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	//sort
	buddy_sort_list( qq );
	put_byte( buf, 0x83 );	//command
	put_byte( buf, 0x00 );
	//start at pos
	int i, j, count=1;
	pthread_mutex_lock( &qq->buddy_list.mutex );
	for( i=0; i<qq->buddy_list.count; i++ )
		if( ((qqbuddy*)qq->buddy_list.items[i])->number >= pos )
			break;
	count = MIN( 50, qq->buddy_list.count-i );
	if( count == 0 ){
		pthread_mutex_unlock( &qq->buddy_list.mutex );
		DBG("signature finished.");
		packetmgr_del_packet( &qq->packetmgr, p );
		return;
	}
	put_byte( buf, count );
	for( j=i; j<i+count; j++ ){
		put_int( buf, ((qqbuddy*)qq->buddy_list.items[j])->number );	//
		put_int( buf, 0 );
	}
	pthread_mutex_unlock( &qq->buddy_list.mutex );
	post_packet( qq, p, SESSION_KEY );
}

#ifndef NO_BUDDY_DETAIL_INFO
void prot_buddy_update_signiture_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	switch( cmd ){
	case 0x83:
		{
			uchar result = get_byte( buf );
			if( result!=0 ){
				DBG("reuslt = %d", result );
				return;
			}
			uint next_pos;
			next_pos = get_int( buf );
			while( buf->pos < buf->len ){
				uint number = get_int( buf );
				qqbuddy* b = buddy_get( qq, number, 0 );
				if( !b ){
					DBG("buddy_get(%d) failed", number);
					return;
				}
				b->sign_time = get_int( buf );
				uchar len = get_byte( buf );
			//	len = MIN( len, SIGNITURE_LEN-1 );
				get_data( buf,  (uchar*)b->signature, len );
				b->signature[len] = 0;
			//	DBG("sign: %s  %s", b->nickname, b->signature );
			}
			if( next_pos != 0 && next_pos != 0xffffffff ){
				//send else
				prot_buddy_update_signiture( qq, next_pos );
			}
		}
		break;
	default:
		DBG("unknown cmd = %x", cmd );
	}
}

void prot_buddy_update_account( struct qqclient* qq, uint pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_ACCOUNT );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	//sort
	buddy_sort_list( qq );
	put_byte( buf, 0x01 );	//command
	put_int( buf, qq->number );
	//start at pos
	int i, j, count;
	pthread_mutex_lock( &qq->buddy_list.mutex );
	for( i=0; i<qq->buddy_list.count; i++ )
		if( ((qqbuddy*)qq->buddy_list.items[i])->number >= pos )
			break;
	count = MIN( 50, qq->buddy_list.count-i );
	if( count == 0 ){
		pthread_mutex_unlock( &qq->buddy_list.mutex );
		DBG("account finished.");
		packetmgr_del_packet( &qq->packetmgr, p );
		return;
	}
	put_byte( buf, count );
	for( j=i; j<i+count; j++ ){
		put_int( buf, ((qqbuddy*)qq->buddy_list.items[j])->number );	//
	}
	pthread_mutex_unlock( &qq->buddy_list.mutex );
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_update_account_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	switch( cmd ){
	case 0x01:
		{
			get_int(buf );	//self number
			ushort result = get_word( buf );
			if( result!=0x0001 ){
				DBG("reuslt = %d", result );
				return;
			}
			uint next_pos;
			next_pos = get_int( buf );
			int count;
			count = get_word( buf );
			while( buf->pos < buf->len ){
				uint number = get_int( buf );
				qqbuddy* b = buddy_get( qq, number, 0 );
				if( !b ){
					DBG("b==NULL");
					return;
				}
				b->account_flag = get_int( buf );
				if( b->account_flag > 0 ){
					uchar len = get_byte( buf );
					len = MIN( len, ACCOUNT_LEN-1 );
					get_data( buf,  (uchar*)b->account, len );
					b->account[len] = 0;
			//		DBG("account: %s  %s", b->nickname, b->account );
				}
			}
			if( next_pos != 0 && next_pos != 0xffffffff ){
				//send else
				prot_buddy_update_account( qq, next_pos );
			}
		}
		break;
	case 0x03:
		{
			uchar type, result;
			uint number;
			type = get_byte( buf );
			number = get_int( buf );
			result = get_byte( buf );
			if( result != 00 ){
				DBG("failed to verify adding buddy msg.");
			}
			DBG("verified adding buddy msg %u result: %d", number, result );
		}
		break;
	default:
		DBG("unknown cmd = %x", cmd );
	}
}


void prot_buddy_update_alias( struct qqclient* qq, int index )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_BUDDY_ALIAS );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x68 );	//command
	put_byte( buf, index );
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_update_alias_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	int count = 0;
	switch( cmd ){
	case 0x68:
		{
			uchar result = get_byte( buf );
			while( buf->pos < buf->len ){
				uint number = get_int( buf );
				qqbuddy* b = buddy_get( qq, number, 0 );
				if( !b ){
					DBG("b==NULL");
					return;
				}
				uchar len = get_byte( buf );
				len = MIN( len, ALIAS_LEN-1 );
				get_data( buf,  (uchar*)b->alias, len );
				b->alias[len] = 0;
				count ++;
			//	DBG("alias: %s  %s", b->nickname, b->alias );
			}
			if( result!=0x01 ){
				DBG("reuslt = %d", result );
				prot_buddy_update_alias( qq, count );
				return;
			}
			buddy_put_event( qq );
		}
		break;
	default:
		DBG("unknown cmd = %x", cmd );
	}
}
#endif

void prot_buddy_request_addbuddy( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_ADDBUDDY_REQUEST );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_int( buf, number );
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_request_addbuddy_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uint number = get_int( buf );
	uchar result = get_byte( buf );
	uchar flag = get_byte( buf );	//verify flag
	if( result == 0 ){
		qq->data.operating_number = number;
		qqbuddy* b = buddy_get( qq, number , 1 );
		if( b ){
			b->verify_flag = flag;
		}
		switch( flag ){
		case 00: //允许任何人
			prot_user_request_token( qq, number, OP_ADDBUDDY, 1, 0 );
			break;
		case 01: //需要验证
			prot_user_request_token( qq, number, OP_ADDBUDDY, 1, 0 );
			break;
		case 02:
		{
			char msg[128];
			sprintf( msg, "[%u]拒绝被任何人加为好友。", number );
			buddy_msg_callback( qq, 100, time(NULL), msg );
			break;
		}
		case 03:	//answer question
		{
			char msg[128];
			sprintf( msg, "[%u]需要回答问题才能加为好友。", number );
			buddy_msg_callback( qq, 100, time(NULL), msg );
			break;
		}
		default:
			DBG("Unknown flag = %x", flag );
		}
	}
}

void prot_buddy_verify_addbuddy( struct qqclient* qq, uchar cmd, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_ADDBUDDY_VERIFY );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, cmd );	//command
	put_int( buf, number );
	put_word( buf, 0x0000 );
	switch( cmd ){
	case 03:	//
		put_byte( buf, 0x00 );
		break;
	case 02:	//
		put_word( buf, qq->data.user_token.len );
		put_data( buf, qq->data.user_token.data, qq->data.user_token.len );
		put_byte( buf, 1 );
		put_byte( buf, 0 );
		put_byte( buf, strlen(qq->data.addbuddy_str) );
		put_data( buf, (uchar*)qq->data.addbuddy_str, strlen(qq->data.addbuddy_str) );
		break;
	case 01:
		put_word( buf, qq->data.user_token.len );
		put_data( buf, qq->data.user_token.data, qq->data.user_token.len );
		put_byte( buf, 0x00 );
		break;
	case 00:	//
		put_word( buf, qq->data.user_token.len );
		put_data( buf, qq->data.user_token.data, qq->data.user_token.len );
		put_byte( buf, 1 );
		put_byte( buf, 0 );
		break;
	}
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_verify_addbuddy_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	uint number;
	number = get_int( buf );
	switch( cmd ){
	case 0x02:
		break;	//wait for reply..
	case 0x00:
		break;
	case 0x03:
	case 0x01:
		{
			uchar result = get_byte( buf );
			if( result == 0x00 ){
				char msg[128];
				DBG("add buddy %u ok!! [cmd=%x]", number, cmd );
				sprintf( msg, "你已经把[%u]添加为好友。", number );
				buddy_msg_callback( qq, 100, time(NULL), msg );
				//refresh buddylist
				buddy_update_list( qq );
			}else{
				DBG("failed to add buddy %u  result=%d", number, result );
			}
		}
		break;
	default:
		DBG("unknown cmd = %x", cmd );
	}
}

void prot_buddy_del_buddy( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_DEL_BUDDY );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, qq->data.user_token.len );
	put_data( buf, qq->data.user_token.data, qq->data.user_token.len );
	char s[16];
	sprintf( s, "%u", number );
	put_data( buf, (uchar*)s, strlen(s) );
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_del_buddy_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	if( get_byte( buf ) != 0 ){
		DBG("Failed to del buddy %u.", qq->data.operating_number );
	}else{
		char msg[128];
		buddy_remove( qq, qq->data.operating_number );
		sprintf( msg, "删除好友[%u]成功。", qq->data.operating_number );
		buddy_msg_callback( qq, 100, time(NULL), msg );
	}
}

//获取业务信息
void prot_buddy_get_extra_info( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GET_BUDDY_EXTRA_INFO );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x03 );
	put_int( buf, number );
	post_packet( qq, p, SESSION_KEY );
	prot_buddy_get_info( qq, number );
}

void prot_buddy_get_extra_info_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	buf = NULL;
}

//获取好友详细资料 
void prot_buddy_get_info( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_BUDDY_INFO );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_word( buf, 0x0001 );
	put_int( buf, number );
	buf->pos += 22;	//22 zeros
	put_word( buf, 0x001A );	//entry count  4E和52是标记分隔符
	put_word( buf, 0x4E22 );	//nickname
	put_word( buf, 0x4E25 );	//邮政编码
	put_word( buf, 0x4E26 );	//地址
	put_word( buf, 0x4E27 );	//家庭电话
	put_word( buf, 0x4E29 );	//
	put_word( buf, 0x4E2A );	//
	put_word( buf, 0x4E2B );	//email
	put_word( buf, 0x4E2C );	//occupation
	put_word( buf, 0x4E2D );	//主页
	put_word( buf, 0x4E2E );	//
	put_word( buf, 0x4E2F );	//
	put_word( buf, 0x4E30 );	//手机
	put_word( buf, 0x4E31 );	//资料可见度
	put_word( buf, 0x4E33 );	//个人说明
	put_word( buf, 0x4E35 );	//毕业学校
	put_word( buf, 0x4E36 );	//
	put_word( buf, 0x4E37 );	//
	put_word( buf, 0x4E38 );	//
	put_word( buf, 0x4E3F );	//
	put_word( buf, 0x4E40 );	//
	put_word( buf, 0x4E41 );	//第1语言
	put_word( buf, 0x4E42 );	//第2语言
	put_word( buf, 0x4E43 );	//第3语言
	put_word( buf, 0x4E45 );	//年龄
	put_word( buf, 0x520B );	//会员信息
	put_word( buf, 0x520F );	//
	post_packet( qq, p, SESSION_KEY );
}

void prot_buddy_get_info_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	qqbuddy* b;
	uint num, ret;
	ushort len, type;
	get_word(buf);	//0x0001
	ret = get_byte(buf);	//result
	if( ret!=00 )
		return;
	num = get_int(buf);
	b = buddy_get( qq, num, 0 );
	if( b==NULL )
		return;
	get_int(buf);
	get_word(buf);	//entry count
	while( buf->pos < buf->len ){
		type = get_word(buf);
		len = get_word(buf);
		switch(type){
		case 0x4E22://nickname
			if( len<NICKNAME_LEN )
				get_data(buf, b->nickname, len);
			break;
		case 0x4E25://post code
			if( len<32 )
				get_data(buf, b->post_code, len);
			break;
		case 0x4E26://address
			if( len<64 )
				get_data(buf, b->address, len);
			break;
		case 0x4E27://homephone
			if( len<32 )
				get_data(buf, b->homephone, len);
			break;
		case 0x4E28://email
			if( len<32 )
				get_data(buf, b->email, len);
			break;
		case 0x4E30://mobile
			if( len<32 )
				get_data(buf, b->mobilephone, len);
			break;
		case 0x4E2C://occupation
			if( len<32 )
				get_data(buf, b->occupation, len);
			break;
		case 0x4E2D://homepage
			if( len<64 )
				get_data(buf, b->homepage, len);
			break;
		case 0x4E33://brief
			if( len<256 )
				get_data(buf, b->brief, len);
			break;
		case 0x4E35://school
			if( len<32 )
				get_data(buf, b->school, len);
			break;
		case 0x4E3F://birth
			if( len<16 )
				get_data(buf, b->birth, len);
			break;
		case 0x4E23://country
			if( len<16 )
				get_data(buf, b->country, len);
			break;
		case 0x4E24://province
			if( len<16 )
				get_data(buf, b->province, len);
			break;
		case 0x4E34://city
			if( len<16 )
				get_data(buf, b->city, len);
			break;
		default:
			buf->pos += len;
		}
	}
}

/*
 *  prot_group.c
 *
 *  QQ Protocol. Part Group
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
#include "qun.h"
#include "group.h"
#include "utf8.h"

/*
void prot_group_download_list( struct qqclient* qq, uint pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GROUP_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x01 );	//command?
	put_byte( buf, 0x01 );
	put_int( buf, 0x0 );	//unknown
	put_int( buf, pos );
	post_packet( qq, p, SESSION_KEY );
}

void prot_group_download_list_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	uchar result = get_byte( buf );
	uint next_pos;
	if( result != 0 ){
		DBG("result = %d", result );
		return ;
	}
	if( cmd == 0x01 ){	//download
		get_int( buf );	//00000000
		next_pos = get_int( buf );
		if( next_pos != 0x00 ){
			prot_group_download_list( qq, next_pos );
		}
		while( buf->pos < buf->len ){
			uint number = get_int( buf );
			uchar type = get_byte( buf );
			uchar gid = get_byte( buf );
			if( type == 0x04 )	//if it is a qun
			{
				qqqun* q = qun_get( qq, number, 1 );
				if( q )
					qun_update_info( qq, q );
			}else if( type == 0x01 ){
				qqbuddy* b = buddy_get( qq, number, 1 );
				if( b ) b->gid = gid / 4;
			}
		}
	}else{
		DBG("unknown cmd=%x", cmd );
	}
}
*/


void prot_group_download_labels( struct qqclient* qq, uint pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_GROUP_LABEL );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x1F );	//command?
	put_byte( buf, 0x01 );
	put_int( buf, pos );
	post_packet( qq, p, SESSION_KEY );
}

void prot_group_download_labels_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	if( cmd == 0x1F ){	//download
		uint next_pos = get_int( buf );
		if( next_pos == 0x1000000 ){	//no group labels info ??
			return;
		}
		if( next_pos != 0x00 ){
			DBG("next_pos == 0x%x", next_pos );
		}
		get_byte( buf );	//unknown
		get_word( buf );
		uchar len;
		uchar order;
		while( buf->pos < buf->len ){
			uchar number = get_byte( buf );
			order = get_byte( buf );
			len = get_byte( buf );
			//temp seems to be utf8 code
			qqgroup *g = group_get( qq, number, 1 );
			if( g == NULL )
				return;
			memset( g->name, 0, NICKNAME_LEN );
			get_data( buf, (uchar*)g->name, len );
			g->order = order;
			DBG("group id: %u  name: %s", g->number, g->name );
		}
		group_put_event( qq );
		buddy_put_event( qq );
	}else{
		DBG("unknown cmd=%x", cmd );
	}
}

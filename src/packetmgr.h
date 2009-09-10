#ifndef _PACKETMGR_H
#define _PACKETMGR_H

#include <pthread.h>
#include "qqdef.h"
#include "loop.h"

#include "qqpacket.h"
struct qqclient;
typedef struct qqpacketmgr{
	int 		recv_packets;
	int 		failed_packets;
	pthread_t 	thread_recv;
	
	loop		ready_loop;
	loop		temp_loop;
	loop		sent_loop;
	
//	ushort		seqno[MAX_COMMAND];
	loop		recv_loop;
}qqpacketmgr;

qqpacket* packetmgr_new_packet( struct qqclient* qq );
qqpacket* packetmgr_new_send( struct qqclient* qq, int cmd );
void packetmgr_del_packet( qqpacketmgr* mgr, qqpacket* p );
void packetmgr_inc_seqno( struct qqclient* qq );
void packetmgr_new_seqno( struct qqclient* qq );
int packetmgr_put( struct qqclient* qq, qqpacket* p );
int packetmgr_put_urge( struct qqclient* qq, qqpacket* p, int urge );
int packetmgr_start( struct qqclient* qq );
void packetmgr_end( struct qqclient* qq );
int packetmgr_check_packet( struct qqclient* qq, int timeout );

#endif

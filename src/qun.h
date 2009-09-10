#ifndef _QUN_H
#define _QUN_H

#include "qqdef.h"

typedef struct qunmember{
	uint 		number;
	char 		nickname[NICKNAME_LEN];
	ushort		face;
	uchar		age;
	uchar		sex;
	uchar		qqshow;
	uchar		flag;
	uchar		status;
	uchar		account_flag;
	char		account[ACCOUNT_LEN];	//email account
	uchar		role, org;
}qunmember;

typedef struct qqqun{
	uint 		number;		//internal number
	uint		ext_number;	//external number
	char		name[NICKNAME_LEN];
	char		ann[256];	//announcement	( too long the word )
	char		intro[256];
	uint		category;
	uint		owner;
	ushort		max_member;
	uchar		auth_type;
	uchar		type;	//qun type
	uint		order;
	token		token_cmd;
	list		member_list;
}qqqun;

struct qqclient;
qqqun* qun_get( struct qqclient* qq, uint number, int create );
qqqun* qun_get_by_ext( struct qqclient* qq, uint ext_number );
void qun_remove( struct qqclient* qq, uint number );
qunmember* qun_member_get( struct qqclient* qq, qqqun* q, uint number, int create );
void qun_member_remove( struct qqclient* qq, qqqun* q, uint number );
void qun_update_memberinfo( struct qqclient* qq, qqqun* q );
void qun_update_info( struct qqclient* qq, qqqun* q );
void qun_update_online( struct qqclient* qq, qqqun* q );
void qun_update_all( struct qqclient* qq );
void qun_set_members_off( struct qqclient* qq, qqqun* q );
void qun_member_cleanup( struct qqclient* qq );
int qun_send_message( struct qqclient* qq, uint number, char* msg );
void qun_put_single_event( struct qqclient* qq, qqqun* q );
void qun_put_event( struct qqclient* qq );
void qun_update_online_all( struct qqclient* qq );

#endif

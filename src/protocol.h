#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "qqdef.h"
#include "qqpacket.h"

struct qqdata_2009{
	uchar		locale[6];
	uchar		version_spec[12];
	uchar		session_key[16];
	uchar		im_key[16];
	uchar		file_key[16];
	uchar		verify_key1[16];
	uchar		verify_key2[16];
	uint		login_info_unknown1;
	uint		login_info_unknown2;
	uchar		exe_hash[16];
	union{
		uchar		server_data[15];
		struct{
			ushort	w_redirect_count;
			uchar	c_redirect_count;
			uint	conn_isp_id;
			uint	server_reverse;
			uint	conn_ip;
		}__attribute__((packed)) server_info;
	};
	token		login_token;
	token		verify_token;
	token		file_token;
	token		token_c;
	token		login_info_token;
	token		login_info_data;
	token		login_info_magic;
	token		login_info_large;
	uchar		login_info_key1[16];
	uchar		login_info_key2[16];
	uchar		login_info_key3[16];
	uchar		login_list_count;	//used for getting list
	token		user_token;
	time_t		user_token_time;
	uint		operating_number;
	uchar		operation;	//当前操作
	char		qqsession[128];
	char		addbuddy_str[128];
};

enum {
	OP_ADDBUDDY = 0x01,
	OP_DELBUDDY
};

enum {
	VF_OK = 0x00,
	VF_VERIFY = 0x01,
	VF_REJECT = 0x02,
	VF_QUESTION = 0x03,
};

enum {
	QQ_IM_TEXT = 0x01,
	QQ_IM_AUTO_REPLY = 0x02
};
enum {
	QQ_RECV_IM_BUDDY_0801 = 0x0009,
	QQ_RECV_IM_TO_UNKNOWN = 0x000a,
	QQ_RECV_IM_NEWS = 0x0018,
	QQ_RECV_IM_UNKNOWN_QUN_IM = 0x0020,
	QQ_RECV_IM_ADD_TO_QUN = 0x0021,
	QQ_RECV_IM_DEL_FROM_QUN = 0x0022,
	QQ_RECV_IM_APPLY_ADD_TO_QUN = 0x0023,
	QQ_RECV_IM_APPROVE_APPLY_ADD_TO_QUN = 0x0024,
	QQ_RECV_IM_REJCT_APPLY_ADD_TO_QUN = 0x0025,
	QQ_RECV_IM_CREATE_QUN = 0x0026,
	QQ_RECV_IM_TEMP_QUN_IM = 0x002A,
	QQ_RECV_IM_QUN_IM = 0x002B,
	QQ_RECV_IM_SYS_NOTIFICATION = 0x0030,
	QQ_RECV_IM_QUN_IM_09 = 0x0052,
	QQ_RECV_IM_BUDDY_0802 = 0x0084,
	QQ_RECV_IM_QUN_MEMBER_IM = 0x008C,
	QQ_RECV_IM_SOMEBODY = 0x008D,
	QQ_RECV_IM_WRITING = 0x0079,
	QQ_RECV_IM_BUDDY_09 = 0x0078,	//09 Preview4 Message
	QQ_RECV_IM_BUDDY_09SP1 = 0x00A6,	//09 SP1 Message，TX协议一变再变，说明什么问题？
	QQ_RECV_IM_UNKNOWN_BUDDY_09 = 0x0031	//09 陌生人
};
enum {
	QQ_NORMAL_IM_TEXT = 0x000b,
	QQ_NORMAL_IM_FILE_REQUEST_TCP = 0x0001,
	QQ_NORMAL_IM_FILE_APPROVE_TCP = 0x0003,
	QQ_NORMAL_IM_FILE_REJECT_TCP = 0x0005,
	QQ_NORMAL_IM_FILE_REQUEST_UDP = 0x0035,
	QQ_NORMAL_IM_FILE_APPROVE_UDP = 0x0037,
	QQ_NORMAL_IM_FILE_REJECT_UDP = 0x0039,
	QQ_NORMAL_IM_FILE_NOTIFY = 0x003b,
	QQ_NORMAL_IM_FILE_PASV = 0x003f,
	QQ_NORMAL_IM_FILE_CANCEL = 0x0049,
	QQ_NORMAL_IM_FILE_EX_REQUEST_UDP = 0x81,
	QQ_NORMAL_IM_FILE_EX_REQUEST_ACCEPT = 0x83,
	QQ_NORMAL_IM_FILE_EX_REQUEST_CANCEL = 0x85,
	QQ_NORMAL_IM_FILE_EX_NOTIFY_IP = 0x87
};

enum KEY_TYPE{
	NO_KEY,
	RANDOM_KEY,
	SESSION_KEY,
	LOGIN_KEY,
	IM_KEY
};

enum VERIFY_MODE{
	VM_NO,
	VM_GET,
	VM_ANSWER
};

enum QQ_CMD{
	QQ_CMD_TOUCH = 			0x0091,
	QQ_CMD_LOGOUT = 		0x0062,	
	QQ_CMD_LOGIN_REQUEST  = 	0x00ba,
	QQ_CMD_LOGIN_GET_INFO = 	0x00e5,	
	QQ_CMD_LOGIN_A4 = 		0x00a4,
	QQ_CMD_LOGIN_GET_LIST = 	0x0018,
	QQ_CMD_LOGIN_SEND_INFO = 	0x0030,
	QQ_CMD_KEEP_ALIVE = 		0x0058,
	QQ_CMD_GET_USER_INFO = 		0x0006,
	QQ_CMD_CHANGE_STATUS = 		0x000d,
	QQ_CMD_SEND_IM = 		0x00cd,
	QQ_CMD_RECV_IM = 		0x0017,
	QQ_CMD_RECV_IM_09 = 		0x00ce,
	QQ_CMD_GET_KEY = 		0x001d,
	QQ_CMD_GET_BUDDY_LIST = 	0x0126,
	QQ_CMD_GET_BUDDY_ONLINE = 	0x0027,
	QQ_CMD_QUN_CMD = 		0x0002,
	QQ_CMD_BUDDY_INFO = 		0x003c,
	QQ_CMD_BUDDY_ALIAS = 		0x003e,
	QQ_CMD_GROUP_LABEL = 		0x0001,
	QQ_CMD_GET_LEVEL = 		0x005C,
	QQ_CMD_GET_BUDDY_EXTRA_INFO = 	0x0065,
	QQ_CMD_GET_BUDDY_SIGN = 	0x0067,
	QQ_CMD_BROADCAST = 		0x0080,
	QQ_CMD_BUDDY_STATUS = 		0x0081,
	QQ_CMD_ADDBUDDY_REQUEST = 	0x00A7,
	QQ_CMD_ADDBUDDY_VERIFY = 	0x00A8,
	QQ_CMD_ADDBUDDY_QUESTION = 	0x00B7,
	QQ_CMD_ACCOUNT = 		0x00b5,
	QQ_CMD_GET_NOTICE = 		0x00d4,
	QQ_CMD_CHECK_IP = 		0x00da,
	QQ_CMD_LOGIN_VERIFY = 		0x00dd,
	QQ_CMD_REQUEST_TOKEN = 		0x00ae,
	QQ_CMD_DEL_BUDDY = 		0x000a
};

struct qqclient;
//prot_login.c
void prot_login_touch( struct qqclient* qq );
void prot_login_touch_with_info( struct qqclient* qq, uchar* server_info, uchar len );
void prot_login_touch_reply( struct qqclient* qq, qqpacket* p );
void prot_login_request( struct qqclient* qq, token* tok, const char* code, char png_data );
void prot_login_request_reply( struct qqclient* qq, qqpacket* p );
void prot_login_verify( struct qqclient* qq );
void prot_login_verify_reply( struct qqclient* qq, qqpacket* p );
void prot_login_get_info( struct qqclient* qq );
void prot_login_get_info_reply( struct qqclient* qq, qqpacket* p );
void prot_login_send_info( struct qqclient* qq );
void prot_login_send_info_reply( struct qqclient* qq, qqpacket* p );
void prot_login_logout( struct qqclient* qq );
void prot_login_a4( struct qqclient* qq );
void prot_login_a4_reply( struct qqclient* qq, qqpacket* p );
void prot_login_get_list( struct qqclient* qq, ushort pos );
void prot_login_get_list_reply( struct qqclient* qq, qqpacket* p );
//prot_misc.c
void prot_misc_broadcast( struct qqclient* qq, qqpacket* p );
//prot_user.c
void prot_user_change_status( struct qqclient* qq );
void prot_user_change_status_reply( struct qqclient* qq, qqpacket* p );
void prot_user_get_notice( struct qqclient* qq, uchar type );
void prot_user_get_notice_reply( struct qqclient* qq, qqpacket* p );
void prot_user_get_key( struct qqclient* qq, uchar key );
void prot_user_get_key_reply( struct qqclient* qq, qqpacket* p );
void prot_user_check_ip( struct qqclient* qq );
void prot_user_check_ip_reply( struct qqclient* qq, qqpacket* p );
void prot_user_keep_alive( struct qqclient* qq );
void prot_user_keep_alive_reply( struct qqclient* qq, qqpacket* p );
void prot_user_get_level( struct qqclient* qq );
void prot_user_get_level_reply( struct qqclient* qq, qqpacket* p );
void prot_user_request_token( struct qqclient* qq, uint number, uchar operation, ushort type, const char* code );
void prot_user_request_token_reply( struct qqclient* qq, qqpacket* p );
//prot_im.c
void prot_im_ack_recv( struct qqclient* qq, qqpacket* pre );
void prot_im_recv_msg( struct qqclient* qq, qqpacket* p );
void prot_im_send_msg( struct qqclient* qq, uint to, char* msg );
void prot_im_send_msg_ex( struct qqclient* qq, uint to, char* msg, int len,
	ushort msg_id, uchar slice_count, uchar which_piece );
//prot_buddy.c
void prot_buddy_get_info( struct qqclient* qq, uint number );
void prot_buddy_get_info_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_get_extra_info( struct qqclient*, uint number );
void prot_buddy_get_extra_info_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_update_list( struct qqclient* qq, ushort pos );
void prot_buddy_update_list_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_update_online( struct qqclient* qq, uint next_number );
void prot_buddy_update_online_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_status( struct qqclient* qq, qqpacket* p );
void prot_buddy_update_signiture( struct qqclient* qq, uint pos );
void prot_buddy_update_signiture_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_update_account( struct qqclient* qq, uint pos );
void prot_buddy_update_account_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_update_alias( struct qqclient* qq, int index );
void prot_buddy_update_alias_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_request_addbuddy( struct qqclient* qq, uint number );
void prot_buddy_request_addbuddy_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_verify_addbuddy( struct qqclient* qq, uchar cmd, uint number );
void prot_buddy_verify_addbuddy_reply( struct qqclient* qq, qqpacket* p );
void prot_buddy_del_buddy( struct qqclient* qq, uint number );
void prot_buddy_del_buddy_reply( struct qqclient* qq, qqpacket* p );
//prot_group.c
void prot_group_download_list( struct qqclient* qq, uint pos );
void prot_group_download_list_reply( struct qqclient* qq, qqpacket* p );
void prot_group_download_labels( struct qqclient* qq, uint pos );
void prot_group_download_labels_reply( struct qqclient* qq, qqpacket* p );
//prot_qun.c
void prot_qun_get_info( struct qqclient* qq, uint number, uint pos );
void prot_qun_send_msg( struct qqclient* qq, uint number, char* msg_content );
void prot_qun_get_memberinfo( struct qqclient* qq, uint number, uint* numbers, int count );
void prot_qun_get_online( struct qqclient* qq, uint number );
void prot_qun_get_membername( struct qqclient* qq, uint number );
void prot_qun_cmd_reply( struct qqclient* qq, qqpacket* p );

int post_packet( struct qqclient* qq, qqpacket* p, int key_type );
int process_packet( struct qqclient* qq, qqpacket* p, bytebuffer* buf );

#endif //_PROTOCOL_H


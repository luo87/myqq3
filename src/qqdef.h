#ifndef _QQDEF_H
#define _QQDEF_H

#include <time.h>
#include <unistd.h>
#include "util.h"
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;


#define UDP 0
#define TCP 1
#define PROXY_HTTP 2

#define MIN(a,b)(a>b?b:a )
#define KB(a) a*1024
#define MB(a) a*KB(1024)
#define CN_TIME(a) a
//#define CN_TIME(a) (a+15*3600) //from New York Time to Beijing Time

#ifdef __WIN32__
#define SLEEP(a)	msleep(a*1000);
#define USLEEP(a)	msleep(a);
#else
#define SLEEP(a)	sleep(a);
#define USLEEP(a)	usleep(a*1000);
#define stricmp		strcasecmp
#endif

//Preference  more in protocol.c: process_packet
//#define NO_BUDDY_INFO	//don't get buddy list or online list
//#define NO_BUDDY_DETAIL_INFO //don't get buddy mail account, alias, signiture info
//#define NO_QUN_INFO //don't get qun info
//#define NO_GROUP_INFO //don't get group info

#define MAX_LOOP_PACKET 32
#define MAX_COMMAND 0x0200
#define MAX_BUDDY 1200	//最大好友个数
#define MAX_QUN	128	//最多群个数
#define MAX_QUN_MEMBER 800	//群最多800成员
#define MAX_GROUP 128	//最多分组个数
#define MAX_EVENT 128	//最多事件缓冲个数
#define USER_INFO_LEN 256
#define MAX_USER_INFO 38
#define SIGNITURE_LEN 256	//100
#define TOKEN_LEN 256		//256
#define ACCOUNT_LEN 64		//
#define NICKNAME_LEN 64		//12
#define GROUPNAME_LEN 256
#define AUTO_REPLY_LEN 256
#define ALIAS_LEN 32		//8
#define PATH_LEN 1024

#define MAX_SERVER_ADDR 16

typedef struct token{
	short	len;
	uchar	data[TOKEN_LEN];
}token;

//#define QQ_VERSION	0x1205	//祈福版
//#define QQ_VERSION	0x115b	//贺岁版
//#define QQ_VERSION	0x1525	//QQ2009Preview4
//#define QQ_VERSION	0x1663	//QQ2009正式版
#define QQ_VERSION	0x1801	//QQ2009 International Beta1
//#define QQ09_LOCALE {0x00,0x00,0x08,0x04,0x01,0xE0}
//#define QQ09_VERSION_SPEC {0x00,0x00,0x02,0x20,0x00,0x00,0x00,0x01,0x00,0x00,0x08,0xFB}
//#define QQ09_EXE_HASH {0xD5,0xCB,0x09,0x9A,0x61,0x63,0x16,0x7E,0xEA,0xF8,0x05,0x6E,0xFF,0x50,0xF3,0x12}
/* QQ2009 International Beta1 */
#define QQ09_LOCALE {0x00,0x00,0x04,0x09,0x01,0xE0}
#define QQ09_VERSION_SPEC {0x00,0x00,0x02,0x20,0x00,0x00,0x00,0x01,0x00,0x00,0x09,0x61}
#define QQ09_EXE_HASH {0x2F,0xFD,0xFE,0x74,0xD8,0x3B,0xB5,0x6A,0x24,0x88,0xBB,0xF8,0x75,0x13,0xC1,0x01}

enum QQSTATUS{
	QQ_NONE = 0x00,
	QQ_ONLINE = 0x0a,
	QQ_OFFLINE = 0x14,
	QQ_AWAY = 0x1e,
	QQ_BUSY = 0x32,
	QQ_KILLME = 0x3C,
	QQ_QUIET = 0x46,
	QQ_HIDDEN = 0x28
};

enum MESSAGE_TYPE{
	MT_BUDDY,
	MT_QUN,
	MT_SYSTEM,
	MT_NEWS,
	MT_QUN_MEMBER
};

#define	MSG_CONTENT_LEN KB(4)
typedef struct qqmessage{
	ushort	msg_id;
	uchar	msg_type;
	ushort	msg_seq;
	char	msg_content[MSG_CONTENT_LEN];
	uint	msg_time;
	uint	qun_number;
	uint	from;
	char	auto_reply;
	char	slice_count;
	char	slice_no;
	ushort	im_type;
}qqmessage;

struct qqclient;
extern void buddy_msg_callback ( struct qqclient* qq, uint uid, time_t t, char* msg );
extern void qun_msg_callback ( struct qqclient* qq, uint uid, uint int_uid,
	time_t t, char* msg );

#endif

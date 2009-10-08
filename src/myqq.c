/*
 *  myqq.c
 *
 *  main() A small console for MyQQ.
 *
 *  Copyright (C) 2008  Huang Guan (gdxxhg@gmail.com)
 *
 *  2008-01-31 Created.
 *  2008-2-5   Add more commands.
 *  2008-7-15  Mr. Wugui said "There's no accident in the world.", 
 *		   but I always see accident in my code :)
 *  2008-10-1  Character color support and nonecho password.
 *  2009-1-25  Use UTF-8 as a default type.
 *
 *  Description: This file mainly includes the functions about 
 *			   Parsing Input Commands.
 *		 Warning: this file should be in UTF-8.
 *			   
 */


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __WIN32__
#include <conio.h>
#include <winsock.h>
#include <wininet.h>
#include <windows.h>
#else
#include <termios.h> //read password
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include "qqclient.h"
#include "buddy.h"
#include "qun.h"
#include "group.h"
#include "memory.h"
#include "utf8.h"
#include "config.h"
#include "qqsocket.h"


#define MSG need_reset = 1; setcolor( color_index ); printf

#define QUN_BUF_SIZE	80*100
#define BUDDY_BUF_SIZE	80*500
#define PRINT_BUF_SIZE	80*500*3


static char* qun_buf, *buddy_buf, *print_buf;
static uint to_uid = 0;		//send message to that guy.
static uint qun_int_uid;		//internal qun id if entered.
static char input[1024];
static int enter = 0;
static qqclient* qq;
static int need_reset, no_color = 0;
enum{
	CMD_EXIT = 0, CMD_EXIT2,
	CMD_SAY, CMD_SAY2,
	CMD_TO, CMD_TO2,
	CMD_HELP,
	CMD_STATUS,
	CMD_ENTER, CMD_ENTER2,
	CMD_LEAVE, CMD_LEAVE2,
	CMD_WHO, CMD_WHO2,
	CMD_VIEW, CMD_VIEW2,
	CMD_QUN, CMD_QUN2,
	CMD_INFO, CMD_INFO2,
	CMD_UPDATE, CMD_UPDATE2,
	CMD_CHANGE, CMD_CHANGE2,
	CMD_TEST,
	CMD_VERIFY, CMD_VERIFY2,
	CMD_ADD, CMD_ADD2,
	CMD_DEL,
	CMD_CLS, CMD_CLS2, CMD_CLS3
};
static char commands[][16]={
	"exit", "x",
	"say", "s",
	"to", "t",
	"help",
	"status",
	"enter", "e",
	"leave", "l",
	"who", "w",
	"view", "v",
	"qun", "q",
	"info", "i",
	"update", "u",
	"change", "c",
	"test",
	"verify", "r",
	"add", "a",
	"del",
	"cls", "clear", "clrscr"
};

static char help_msg[]=
"欢迎使用 MyQQ2009 中文版\n"
"这是一个为程序员和电脑爱好者制作的"
"迷你控制台即时通讯软件,享受它吧!\n"
"help:	  显示以下帮助信息.\n"
"add/a:	 添加好友. add+QQ号码.\n"
"cls/clear: 清屏.\n"
"view/v:	所有好(群)友列表.(指向前操作)\n"
"who/w:	 在线好(群)友列表.(指向前操作)\n"
"qun/q:	 显示群列表.(指向前操作)\n"
"to/t:	  指向某个QQ号或者前面的编号.\n"
"enter/e:   指向某个群号或者前面的编号.\n"
"leave/l:   离开群.(指向后操作)\n"
"say/s:	 发送信息.(指向后操作)\n"
"info/i:	查看相应信息.(指向后操作)\n"
"update/u:  更新所有列表.\n"
"status:	改变状态(online|away|busy|killme|hidden)\n"
"verify/r:  输入验证码(验证码在verify目录下).\n"
"change/c:  更换用户登陆.\n"
"exit/x:	退出.\n"
;

#ifdef __WIN32__
#define CCOL_GREEN	FOREGROUND_GREEN
#define CCOL_LIGHTBLUE	FOREGROUND_BLUE | FOREGROUND_GREEN
#define CCOL_REDGREEN	FOREGROUND_RED | FOREGROUND_GREEN
#define CCOL_NONE		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
static int color_index = CCOL_NONE; 
static void charcolor( int col )
{
	color_index = col;
}
static void setcolor( int col )
{
	if(!no_color)
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_INTENSITY | col);
}
static void clear_screen()
{
	system("cls");
}
#else

#define CCOL_NONE		"\033[0m"   
#define CCOL_BLACK	"\033[0;30m"   
#define CCOL_DARKGRAY	"\033[1;30m"   
#define CCOL_BLUE		"\033[0;34m"   
#define CCOL_LIGHTBLUE	"\033[1;34m"   
#define CCOL_GREEN	"\033[0;32m"   
#define CCOL_LIGHTGREEN	"\033[1;32m"   
#define CCOL_CYAN		"\033[0;36m"   
#define CCOL_LIGHTCYAN	"\033[1;36m"   
#define CCOL_RED		"\033[0;31m"   
#define CCOL_LIGHTRED	"\033[1;31m"   
#define CCOL_PURPLE	"\033[0;35m"   
#define CCOL_LIGHTPURPLE	"\033[1;35m"   
#define CCOL_LIGHTBROWN	"\033[0;33m"   
#define CCOL_YELLOW	"\033[1;33m"   
#define CCOL_LIGHTGRAY	"\033[0;37m"   
#define CCOL_WHITE	"\033[1;37m"
#define CCOL_REDGREEN	CCOL_YELLOW
static char* color_index = CCOL_NONE;
static void charcolor( const char* col )
{
	color_index = (char*)col;
}
static void setcolor( const char* col )
{
	if(!no_color)
		printf( col );
}
static void clear_screen()
{
	//system("clear");
	printf("\033[2J   \033[0;0f");
}
#endif

//含颜色控制 
#define RESET_INPUT \
	if( need_reset ){\
		charcolor( CCOL_NONE );\
	if( enter ){ \
		MSG("In {%s}> ", _TEXT( myqq_get_qun_name( qq, qun_int_uid ) ) ); \
	}else{ \
		MSG("To [%s]> ", _TEXT( myqq_get_buddy_name( qq, to_uid ) ) );} \
	fflush( stdout ); \
	need_reset = 0;}




#ifdef __WIN32__
#define _TEXT to_gb_force 
static char* to_gb_force( char* s )
{
//	memset( print_buf, 0, PRINT_BUF_SIZE );
	utf8_to_gb( s, print_buf, PRINT_BUF_SIZE-1 );
	return print_buf;
}
static char* to_utf8( char* s )
{
//	memset( print_buf, 0, PRINT_BUF_SIZE );
	gb_to_utf8( s, print_buf, PRINT_BUF_SIZE-1 );
	return print_buf;
}

#else
#define _TEXT
#endif
static int getline(char *s, int lim) {
	char *t;
	int c;

	t = s;
	while (--lim>1 && (c=getchar()) != EOF && c != '\n')
		*s++ = c;
	*s = '\0';
	return s - t;
}

static char* mode_string( int mode )
{
	switch( mode ){
	case QQ_ONLINE:
		return "Online";
	case QQ_AWAY:
		return "Away";
	case QQ_BUSY:
		return "Busy";
	case QQ_OFFLINE:
		return "Offline";
	case QQ_HIDDEN:
		return "Hidden";
	case QQ_KILLME:
		return "Kill Me";
	case QQ_QUIET:
		return "Quiet";
	}
	return "Unknown";
}
static char* skip_line( char* p, int ln )
{
	while( *p && ln-- )
	{
		p ++;
		while( *p && *p!='\n' ) p++;
	}
	return p;
}
static char* myqq_get_buddy_name( qqclient* qq, uint uid )
{
	static char tmp[16];
	qqbuddy* b = buddy_get( qq, uid, 0 );
	if( b )
		return b->nickname;
	if( uid == 10000 )
		return "System";
	if( uid != 0 ){
		sprintf( tmp, "%u" , uid );
		return tmp;
	}
	return "Nobody";
}
static char* myqq_get_qun_name( qqclient* qq, uint uid )
{
	static char tmp[16];
	qqqun* q = qun_get( qq, uid, 1 );
	if( q )
		return q->name;
	if( uid != 0 ){
		sprintf( tmp, "%u" , uid );
		return tmp;
	}
	return "[q==NULL]";
}
static char* myqq_get_qun_member_name( qqclient* qq, uint int_uid, uint uid )
{
	static char tmp[16];
	qqqun* q = qun_get( qq, int_uid, 0 );
	if( q ){
		qunmember* m = qun_member_get( qq, q, uid, 0 );
		if( m )
			return m->nickname;
		if( uid != 0 ){
			sprintf( tmp, "%u" , uid );
			return tmp;
		}
		return "[m==NULL]";
	}
	return "[q==NULL]";
}
static int myqq_send_im_to_qun( qqclient* qq, uint int_uid, char* msg, int wait )
{
	qun_send_message( qq, int_uid, msg );
	if( wait )
	{
		if( qqclient_wait( qq, 15 ) < 0 )
			return -1;
	}
	return 0;
}
static int myqq_send_im_to_buddy( qqclient* qq, uint int_uid, char* msg, int wait )
{
	buddy_send_message( qq, int_uid, msg );
	if( wait )
	{
		if( qqclient_wait( qq, 15 ) < 0 )
			return -1;
	}
	return 0;
}

static int myqq_get_buddy_info( qqclient* qq, uint uid, char* buf, int size )
{
	qqbuddy *b = buddy_get( qq, uid, 0 );
	if( size < 256 )
		return -1;
	if( b == NULL )
		return -2;
	int len;
	len = sprintf( buf,	"好友昵称\t%s\n"
				"用户账号\t%d\n"
				"签名\t\t%s\n"
				"备注\t\t%s\n"
				"手机\t\t%s\n"
				"邮箱\t\t%s\n"
				"职业\t\t%s\n"
				"主页\t\t%s\n"
				"毕业学院\t%s\n"
				"来自\t\t%s %s %s\n"
				"通讯地址\t%s\n"
				"自我介绍\t%s\n"
				"头像\t\t%d\n"
				"年龄\t\t%d\n"
				"性别\t\t%s\n"
				"状态\t\t%s\n",
		b->nickname, b->number, b->signature, b->alias, b->mobilephone, 
		b->email, b->occupation, b->homepage, b->school, b->country, b->province, b->city,
		b->address, b->brief, b->face, b->age,
		(b->sex==0x00)?"Male": (b->sex==0x01)?"Female":"Asexual", mode_string(b->status) );
	return len;
}


//Note: this function change the source string directly.
static char* util_escape( char* str )
{
	unsigned char* ch;
	ch = (unsigned char*)str;
	while( *ch )
	{
		if( *ch < 0x80 ) //ASCII??
		{
			if( !isprint(*ch) ) //if not printable??
				*ch = ' ';	//use space instead..
			ch ++;	//skip one
		}else{
			ch +=2; //skip two
		}
	}
	return str;
}

/*   The output buf looks like this:
L8bit  L16bit		L16bit		L32bit
1	  357339036	 online		Xiao xia
2	  273310179	 offline	Huang Guan
*/
//Note: size must be big enough
static int myqq_get_buddy_list( qqclient* qq, char* buf, int size, char online )
{
	int i, len = 0;
	//avoid editing the array
	buf[0] = 0;
	pthread_mutex_lock( &qq->buddy_list.mutex );
	int ln = 1;
	for( i=0; i<qq->buddy_list.count; i++ )
	{
		qqbuddy * b = (qqbuddy*)qq->buddy_list.items[i];
		if( online && b->status == QQ_OFFLINE ) continue;
		if( *b->alias ){
			len = sprintf( buf, "%s%-8d%-16d%-16s%-16.64s\n", buf, ln ++, b->number, 
				mode_string( (int) b->status ), util_escape( b->alias ) );
		}else{
			len = sprintf( buf, "%s%-8d%-16d%-16s%-16.64s\n", buf, ln ++, b->number, 
				mode_string( (int) b->status ), util_escape( b->nickname ) );
		}
		if( len + 80 > size ) break;
	}
	pthread_mutex_unlock( &qq->buddy_list.mutex );
	return len;
}

/*   The output buf looks like this:
L8bit  L16bit		L16bit		L32bit
1	  467788923	 12118724	Xinxing Experimental School
2	  489234223	 13223423	SGOS2007
*/
//Note: size must be big enough
static int myqq_get_qun_list( qqclient* qq, char* buf, int size )
{
	int i, len = 0, ln=1;
	//avoid editing the array
	buf[0] = 0;
	pthread_mutex_lock( &qq->qun_list.mutex );
	for( i=0; i<qq->qun_list.count; i++ )
	{
		qqqun * q = (qqqun *)qq->qun_list.items[i];
		len = sprintf( buf, "%s%-8d%-16d%-16d%-16.64s\n", buf, ln ++, q->number, 
			q->ext_number, util_escape( q->name ) );
		if( len + 80 > size ) break;
	}
	pthread_mutex_unlock( &qq->qun_list.mutex );
	return len;
}

/*   The output buf looks like this:
L8bit  L16bit		L16bit		L32bit
1	  357339036	 Manager	Xiaoxia
2	  273310179	 Fellow		Huang Guan
*/
//Note: size must be big enough
static int myqq_get_qun_member_list( qqclient* qq, uint int_uid, char* buf, int size, char online )
{
	qqqun * q = qun_get( qq, int_uid, 0 );
	if( !q )return 0;
	//Hope the Qun won't get removed while we are using it. 
	int i, len = 0, ln = 1;
	buf[0] = 0;
	pthread_mutex_lock( &q->member_list.mutex );
	for( i=0; i<q->member_list.count; i++ )
	{
		qunmember * m = (qunmember *)q->member_list.items[i];
		if( online && m->status == QQ_OFFLINE ) continue;
		len = sprintf( buf, "%s%-8d%-16d%-16s%-16.64s\n", buf, ln++, m->number, 
			(m->role&1)?"Admin":"Fellow", util_escape( m->nickname ) );
		if( len + 80 > size )
			break;
	}
	pthread_mutex_unlock( &q->member_list.mutex );
	return len;
}

//interface for getting qun information
/* Output style:
*/
static int myqq_get_qun_info( qqclient* qq, uint int_uid, char* buf, int size )
{
	char cate_str[4][10] = {"Classmate", "Friend", "Workmate", "Other" };
	qqqun *q = qun_get( qq, int_uid, 0 );
	if( !q )return 0;
	int len;
	if( size < 256 )
		return -1;

	if( q == NULL )
		return -2;
	len = sprintf( buf, 	"名称\t\t%s\n"
				"内部号码\t%d\n"
				"群号码\t\t%d\n"
				"群类型\t\t%s\n"
				"加入验证\t%s\n"
				"群分类\t\t%s\n"
				"创建人\t\t%d\n"
				"成员数量\t%d\n"
				"群的简介\n%s\n"
				"群的公告\n%s\n",
		q->name, q->number, q->ext_number, (q->type==0x01)?"Normal":"Special",
		(q->auth_type==0x01)?"No": (q->auth_type==0x02)?"Yes":
			(q->auth_type==0x03)?"RejectAll":"Unknown",
		q->category > 3 ? cate_str[3] : cate_str[(int)q->category],
		q->owner, q->member_list.count, q->intro, q->ann );
	return len;
}

//含颜色控制
void buddy_msg_callback ( qqclient* qq, uint uid, time_t t, char* msg )
{
	charcolor( CCOL_LIGHTBLUE );	//red color
	char timestr[12];
	char msgstr[64];
	struct tm * timeinfo;
	char* nick = myqq_get_buddy_name( qq, uid );
  	timeinfo = localtime ( &t );
	strftime( timestr, 12, "%H:%M:%S", timeinfo );
	char tmp[20], *p;
	sprintf( tmp, "%-16d", uid );
	p = strstr( buddy_buf, tmp );
	if( p )
	{
		p -= 8;
		if( p>=buddy_buf )
		{
			int ln;
			sscanf( p, "%d", &ln );
			sprintf( msgstr, "\n%d)%s[", ln, timestr );
			strcat( msgstr, nick );
			strcat( msgstr, "]\n\t" );//二次修改 
			MSG( _TEXT( msgstr ) );
			puts( _TEXT( msg ) );
			RESET_INPUT
		}
	}else{
		sprintf( msgstr, "\n%s[", timestr );
		strcat( msgstr, nick );
		strcat( msgstr, "]\n\t" );//二次修改 
		MSG( _TEXT( msgstr ) );
		puts( _TEXT( msg ) );
		RESET_INPUT
	}
}

//含颜色控制
void qun_msg_callback ( qqclient* qq, uint uid, uint int_uid,
	time_t t, char* msg )
{
	charcolor( CCOL_REDGREEN );	//red color
	char timestr[12];
	char msgstr[64];
	char* qun_name = myqq_get_qun_name( qq, int_uid );
	char* nick = myqq_get_qun_member_name( qq, int_uid, uid );
	struct tm * timeinfo;
  	timeinfo = localtime ( &t );
	strftime( timestr, 12, "%H:%M:%S", timeinfo );
	char tmp[20], *p;
	sprintf( tmp, "%-16d", int_uid );
	p = strstr( qun_buf, tmp );
	if( p )
	{
		p -= 8;
		if( p>=qun_buf )
		{
			int ln;
			sscanf( p, "%d", &ln );
			sprintf( msgstr, "\n%d)%s{", ln, timestr );
			strcat( msgstr, qun_name );
			strcat( msgstr, "}[" );
			strcat( msgstr, nick );
			strcat( msgstr, "]\n\t" );//二次修改
			MSG( _TEXT( msgstr ) );
			puts( _TEXT( msg ) );
			RESET_INPUT
		}
	}else{
		sprintf( msgstr, "\n%s{", timestr );
		strcat( msgstr, qun_name );
		strcat( msgstr, "}[" );
		strcat( msgstr, nick );
		strcat( msgstr, "]\n\t" );//二次修改
		MSG( _TEXT( msgstr ) );
		puts( _TEXT( msg ) );
		RESET_INPUT
	}
}

#ifndef __WIN32__
int read_password(char   *lineptr )   
{   
	struct   termios   old,   new;   
	int   nread;	
	/*   Turn   echoing   off   and   fail   if   we   can't.   */   
	if   (tcgetattr   (fileno   (stdout),   &old)   !=   0)   
		return   -1;   
	new   =   old;   
	new.c_lflag   &=   ~ECHO;   
	if   (tcsetattr   (fileno   (stdout),   TCSAFLUSH,   &new)   !=   0)   
		return   -1;
	/*   Read   the   password.   */   
	nread   =   scanf   ("%31s", lineptr);	
	/*   Restore   terminal.   */   
	(void)   tcsetattr   (fileno   (stdout),   TCSAFLUSH,   &old); 	
	return   nread;   
}
#endif

int main(int argc, char** argv)
{
	int cmdid, lastcmd=-1, len;
	char cmd[16], arg[1008];
	srand(time(NULL));
	//init data
	config_init();
	qqsocket_init();
	//no color?
	if( config_readint( g_conf, "NoColor" ) )
		no_color = 1;
	NEW( qun_buf, QUN_BUF_SIZE );
	NEW( buddy_buf, BUDDY_BUF_SIZE );
	NEW( print_buf, PRINT_BUF_SIZE );
	NEW( qq, sizeof(qqclient) );
	if( !qun_buf || !buddy_buf || !print_buf || !qq ){
		MSG("no enough memory.");
		return -1;
	}
	//login
	clear_screen();
DO_LOGIN:
	if( argc<3 )
	{
		uint uid;
		char password[32];
		MSG(_TEXT("QQ账号:"));
		scanf("%u", &uid );
		MSG(_TEXT("QQ密码:"));
#ifdef __WIN32__
		uint pwi;
		char pswd;
		for(pwi=0;pwi<=32;pwi++)
		{
			pswd = getch(); //逐次赋值,但不回显 
			if(pswd == '\x0d')//回车则终止循环
			{
				password[pwi] ='\0';//getch需要补'\0'以适合原程序 
				break;
			}
			if(pswd == '\x08')//删除键则重输QQ密码 
			{
				if( pwi>0 ) pwi=-1;
				MSG(_TEXT("\n请重输QQ密码:"));
				continue;
			}
			printf("*"); //以星号代替字符
			password[pwi] =pswd;
		}
#else	//LINUX
		read_password( password );
#endif
		MSG(_TEXT("\n是否隐身登陆？(y/n)"));
		scanf("%s", input );
		//check if failed here...
		qqclient_create( qq, uid, password );
		qq->mode = *input=='y' ? QQ_HIDDEN : QQ_ONLINE;
		qqclient_login( qq );
		scanf("%c", input ); //If I don't add this, it will display '>' twice.
	}else{
		//check if failed here...
		qqclient_create( qq, atoi(argv[1]), argv[2] );
		if( argc > 3 )
			qq->mode = atoi(argv[3])!=0 ? QQ_HIDDEN : QQ_ONLINE;
		qqclient_login( qq );
		argc = 1;
	}
	MSG(_TEXT("登陆中...\n"));
	for( ; qq->process != P_LOGIN; 
		qqclient_wait( qq, 1 ) /*wait one second*/ ){
		switch( qq->process ){
		case P_LOGGING:
			continue;
		case P_VERIFYING:
			MSG(_TEXT("请输入验证码（验证码目录下）: "));
			scanf( "%s", input );
			qqclient_verify( qq, input );
			break;
		case P_ERROR:
			MSG(_TEXT("网络错误.\n"));
			qqclient_logout( qq );
			qqclient_cleanup( qq );
			goto DO_LOGIN;
		case P_DENIED:
			MSG(_TEXT("您的QQ需要激活(http://jihuo.qq.com).\n"));
#ifdef __WIN32__
			ShellExecute(NULL,"open","http://jihuo.qq.com/",NULL,NULL,SW_SHOWNORMAL);
#endif
			qqclient_logout( qq );
			qqclient_cleanup( qq );
			goto DO_LOGIN;
		case P_WRONGPASS:
			MSG(_TEXT("您的密码错误.\n"));
			qqclient_logout( qq );
			qqclient_cleanup( qq );
			goto DO_LOGIN;
		default:
			MSG(_TEXT("未知错误.\n"));
		}
		
	}
	/* Success */
	MSG( _TEXT(help_msg) );
	while( qq->process != P_INIT ){
		RESET_INPUT
		len = getline( input, 1023 );
		if( len < 1 ) continue;
		char* sp = strchr( input, ' ' );
		if( sp ){
			*sp = '\0';
			strncpy( cmd, input, 16-1 );
			strncpy( arg, sp+1, 1008-1 );
			*sp = ' ';
		}else{
			strncpy( cmd, input, 16-1 );
			arg[0] = '\0';
		}
		need_reset = 1;
		for( cmdid=0; cmdid<sizeof(commands)/16; cmdid++ )
			if( strcmp( commands[cmdid], cmd )==0 )
				break;
SELECT_CMD:
		switch( cmdid ){
		case CMD_TO:
		case CMD_TO2:
		{
			if( enter )
			{
				MSG(_TEXT("您在一个群中, 你可以和任何人谈话.\n"));
				break;
			}
			int n = atoi( arg );
			if( n < 0xFFFF )
			{
				char *p;
				p = skip_line( buddy_buf, n-1 );
				if( p )
				{
					sscanf( p, "%u%u", &n, &to_uid );
					sprintf( print_buf, "您将和 %s 进行谈话\n", myqq_get_buddy_name(qq, to_uid) );
					MSG( _TEXT(print_buf) );
					break;
				}
			}else{
				to_uid = n;
				sprintf( print_buf, "您将和 %s 进行谈话\n", myqq_get_buddy_name(qq, to_uid) );
				MSG( _TEXT(print_buf) );
				break;
			}
			sprintf( print_buf, "to: %s 没有找到.\n", arg );
			MSG( _TEXT(print_buf) );
			break;
		}
		case CMD_SAY:
		case CMD_SAY2:
		{
			if( enter )
			{
#ifdef	__WIN32__
				if( myqq_send_im_to_qun( qq, qun_int_uid, to_utf8(arg), 1 ) < 0 ){
#else
				if( myqq_send_im_to_qun( qq, qun_int_uid, arg, 1 ) < 0 ){
#endif
					MSG(_TEXT("超时: 您的消息发送失败.\n"));
				}
			}else{
				if( to_uid == 0 )
				{
					MSG(_TEXT("say: 和谁谈话?\n"));
					break;
				}
#ifdef	__WIN32__
				if( myqq_send_im_to_buddy( qq, to_uid, to_utf8(arg), 1 ) < 0 ){
#else
				if( myqq_send_im_to_buddy( qq, to_uid, arg, 1 ) < 0 ){
#endif
					MSG(_TEXT("超时: 您的消息发送失败.\n"));
				}
			}
			break;
		}
		case CMD_EXIT:
		case CMD_EXIT2:
			goto end;
		case CMD_HELP:
			MSG( _TEXT(help_msg) );
			break;
		case CMD_CLS:
		case CMD_CLS2:
		case CMD_CLS3:
			clear_screen();
		case CMD_STATUS:
			if( strcmp( arg, "away") == 0 )
				qqclient_change_status( qq, QQ_AWAY );
			else if( strcmp( arg, "online") == 0 )
				qqclient_change_status( qq, QQ_ONLINE );
			else if( strcmp( arg, "hidden") == 0 )
				qqclient_change_status( qq, QQ_HIDDEN );
			else if( strcmp( arg, "killme") == 0 )
				qqclient_change_status( qq, QQ_KILLME );
			else if( strcmp( arg, "busy") == 0 )
				qqclient_change_status( qq, QQ_BUSY );
			else{
				MSG(_TEXT("未知状态\n") );
			}
			break;
		case CMD_ENTER:
		case CMD_ENTER2:
		{
			int n = atoi( arg );
			if( n < 0xFFFF )
			{
				char *p;
				p = skip_line( qun_buf, n-1 );
				if( p )
				{
					sscanf( p, "%u%u", &n, &qun_int_uid );
					sprintf( print_buf, "您在 %s 群中\n", myqq_get_qun_name( qq, qun_int_uid) );
					MSG( _TEXT(print_buf) );
					enter = 1;
					break;
				}
			}else{
				qun_int_uid = n;
				sprintf( print_buf, "您在 %s 群中\n", myqq_get_qun_name( qq, qun_int_uid) );
				MSG( _TEXT(print_buf) );
				enter = 1;
				break;
			}
			sprintf( print_buf, "enter: %s 没有找到.\n", arg );
			MSG( _TEXT(print_buf) );
			break;
		}
		case CMD_LEAVE:
		case CMD_LEAVE2:
			if( !enter )
			{
				MSG(_TEXT("您没有进入群.\n"));
				break;
			}
			enter = 0;
			sprintf( print_buf, "离开 %s. 您将和 %s 进行谈话\n", 
				myqq_get_qun_name( qq, qun_int_uid ), myqq_get_buddy_name( qq, to_uid ) );
			MSG( _TEXT(print_buf) );
			break;
		case CMD_QUN:
		case CMD_QUN2:
		{
			myqq_get_qun_list( qq, qun_buf, QUN_BUF_SIZE );
			MSG( _TEXT( qun_buf ) );
			break;
		}
		case CMD_UPDATE:
		case CMD_UPDATE2:
			qun_update_all( qq );
			buddy_update_list( qq );
			group_update_list( qq );
			MSG(_TEXT("更新中...\n"));
			if( qqclient_wait( qq, 20 )<0 ){
				MSG(_TEXT("更新超时.\n"));
			}
			break;
		case CMD_INFO:
		case CMD_INFO2:
		{
			if( !enter )
			{
				if( to_uid==0 )
				{
					MSG(_TEXT("请先选择一个好友.\n"));
				}else{
					char* buf = (char*)malloc(1024*4);	//4kb enough!
					//update single buddy info
					buddy_update_info( qq, buddy_get( qq, to_uid, 0 ) );
					if( qqclient_wait( qq, 10 ) < 0 ){
						MSG( _TEXT("获取好友详细资料失败。\n") );
					}
					if( myqq_get_buddy_info( qq, to_uid, buf, 1024*4 ) < 0 ){
						sprintf( print_buf, "获取 %s 的信息失败\n", myqq_get_buddy_name( qq, to_uid ) );
						MSG( _TEXT(print_buf) );
					}else{
						MSG( _TEXT( buf ) );
					}
					free(buf);
				}
			}else{
				char* buf = (char*)malloc(1024*4);	//4kb enough!
				if( myqq_get_qun_info( qq, qun_int_uid, buf, 1024*4 ) < 0 ){
					sprintf( print_buf, "获取 %s 的信息失败\n", myqq_get_qun_name( qq, qun_int_uid ) );
					MSG( _TEXT(print_buf) );
				}else{
					MSG( _TEXT( buf ) );
				}
				free(buf);
			}
			break;
		}
		case CMD_VIEW:
		case CMD_VIEW2:
			if( enter )
			{
				myqq_get_qun_member_list( qq, qun_int_uid, buddy_buf,
					BUDDY_BUF_SIZE, 0 );
				MSG( _TEXT( buddy_buf ) );
			}else{
				myqq_get_buddy_list( qq, buddy_buf, BUDDY_BUF_SIZE, 0 );
				MSG( _TEXT( buddy_buf ) );
			}
			break;
		case CMD_WHO:
		case CMD_WHO2:
			if( enter )
			{
				myqq_get_qun_member_list( qq, qun_int_uid, buddy_buf,
					QUN_BUF_SIZE, 1 );
				MSG( _TEXT( buddy_buf ) );
			}else{
				myqq_get_buddy_list( qq, buddy_buf, QUN_BUF_SIZE, 1 );
				MSG( _TEXT( buddy_buf ) );
			}
			break;
		case CMD_CHANGE:
		case CMD_CHANGE2:
			qqclient_logout( qq );
			qqclient_cleanup( qq );
			main( 0, NULL );
			goto end;
		case CMD_VERIFY:
		case CMD_VERIFY2:
			qqclient_verify( qq, arg );
			break;
		case CMD_ADD:
		case CMD_ADD2:
		{
			sprintf( print_buf, "添加[%d]的附言（默认空）：", atoi(arg) );
			MSG( _TEXT(print_buf) );
			getline( input, 50 );
			qqclient_add( qq, atoi(arg), input );
			break;
		}
		case CMD_DEL:
			qqclient_del( qq, atoi(arg) );
			break;
		default:
			//use it as the last cmd's argument
			if( lastcmd && *input )
			{
				cmdid = lastcmd;
				strncpy( arg, input, 1008-1 );
				*input = 0;
				goto SELECT_CMD;
			}
			break;
		}
		lastcmd = cmdid;
	}
end:
	qqclient_logout( qq );
	qqclient_cleanup( qq );
	config_end();
	DEL( qq );
	MSG(_TEXT("离开.\n"));
	DEL( qun_buf );
	DEL( buddy_buf );
	DEL( print_buf );
	setcolor( CCOL_NONE );
	memory_end();
	return 0;
}

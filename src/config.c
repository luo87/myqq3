/*
 *  coinfig.c
 *
 *  Configuration Procedures
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-9 15:40:05 Created.
 *  2009-7-5 23:32:00 Patched a chars memory bug in parse().
 *
 *  Description: This file mainly includes the functions about 
 *  reading configuration file.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "qqdef.h"
#include "memory.h"
#include "config.h"

#ifndef __WIN32__
#define stricmp strcasecmp
#endif

typedef config_item item;

enum PARSING{
	NAME,
	VALUE
};

static void add( config* c, char* name, char* value )
{
	if( c->item_count >= 1024 ){
		DBG("failed to add config item: too many.");
		return;
	}
	int i = c->item_count ++;
	c->items[i] = (config_item*)malloc( sizeof(config_item) );
	strncpy( c->items[i]->name, name, CONFIG_NAME_LEN );
	strncpy( c->items[i]->value, value, CONFIG_VALUE_LEN );
//	printf("added item '%s':'%s'\n", name, value );
}

static void parse( config*conf, char* buf, int len )
{
	enum PARSING state;
	state = NAME;
	int i, j;
	char c;
	char name[CONFIG_NAME_LEN+1], value[CONFIG_VALUE_LEN+1];
	i = j = 0;
	buf[len] = '\0';
	while( i <= len ){
		c = buf[i];
		switch( state ){
		case NAME:
			if( c == '\r' ){
				break;
			}else if( c == '\n' ){ //not found =, but found \n
				j = 0;
			}else if( c == '=' ){
				state = VALUE;
				while( j>0 && name[j-1] == ' ' )
					j--;
				name[j] = '\0';	//add end
				j = 0;
			}else if( c == '\\' ){
				i ++;
			}else{
				if( j==0 && c == ' ' )	//do not add front space
					break;
				if( j<CONFIG_NAME_LEN )
					name[j++] = c;
			}
			break;
		case VALUE:
			if( c == '\r' ){
				break;
			}else if( c == '\n' || c == '\0' ){
				state = NAME;
				while( j>0 && value[j-1] == ' ' )
					j--;
				value[j] = '\0';	//add end
				if( name[0] != '#' ){ //comment?? do not add it.
					add( conf, name, value );
				}
				j = 0;
			}else if( c == '\\' ){
				i ++;
			}else{
				if( j==0 && c == ' ' )	//do not add front space
					break;
				if( j<CONFIG_VALUE_LEN )
					value[j++] = c;
			}
			break;
		default:
			break;
		}
		i ++;
	}
}

static char default_configs[]="### Default Configuration ###\n"
"QQPacketLog = false\n"
"QQTerminalLog = false\n"
"# 日志目录，删除下面一条将不记录日志。\n"
"QQLogDir = ./log\n"
"# 验证码目录\n"
"QQVerifyDir = ./verify\n"
"# 登录方式 UDP or TCP or PROXY_HTTP\n"
"QQNetwork = UDP\n"
"NO_COLOR = false\n"
"下面为QQ服务器列表，每个服务器用|来隔开，IP和端口用:来给开。\n"
"电信服务器\n"
"#QQTcpServerList = 219.133.60.173:80|219.133.38.232:80|219.133.40.177:80\n"
"最多允许添加16个\n"
"QQTcpServerList = 219.133.60.173:443|219.133.49.125:443|58.60.15.33:443\n"
"QQUdpServerList = 219.133.49.171:8000|58.60.14.37:8000|219.133.60.36:8000|58.60.15.39:8000|sz6.tencent.com:8000|sz7.tencent.com:8000\n"
"QQHttpProxyServerList = \n"
;

int config_open( config* c, char* filename )
{
	FILE* fp;
	fp = fopen ( filename, "r" );
	if( fp == NULL ){
		DBG("failed to open file %s", filename );
		fp = fopen( filename, "w" );
		if( fp == NULL )
			return -1;
		fputs( default_configs, fp );
		fclose( fp );
		fp = fopen( filename, "r" );
		if( fp == NULL )
			return -1;
	}
	char* buf;
	buf = (char*) malloc( KB(32) );
	int len = fread( buf, 1, KB(32), fp );
	if( len > 0 ){
		memset( c, 0, sizeof(config) );
		parse( c, buf, len );
	}else{
		DBG("failed to read file %s", filename );
		fclose( fp );
		return -2;
	}
	free( buf );
	fclose( fp );
	return 0;
}

int config_readint( config*c, char* name )
{
	int i;
	for( i=0; i<c->item_count; i++ )
	{
		if( strcmp( c->items[i]->name, name ) == 0 )
		{
			if( stricmp( c->items[i]->value, "true" ) == 0 )
				return 1;
			else if( stricmp( c->items[i]->value, "false" ) == 0 )
				return 0;
			//else
			return atol( c->items[i]->value );
		}
	}
//	DBG("config item not found.");
	return 0;
}

char* config_readstr( config*c, char* name )
{
	int i;
	for( i=0; i<c->item_count; i++ )
	{
		if( strcmp( c->items[i]->name, name ) == 0 )
		{
			return c->items[i]->value;
		}
	}
//	DBG("config item not found.");
	return NULL;
}

void config_close( config* c )
{
	int i;
	if( !c ) return;
	for( i=0; i<c->item_count; i++ )
		free( c->items[i] );
	c->item_count = 0;
}


config *g_conf;
void config_init()
{
	NEW( g_conf, sizeof(config) );
	if( !g_conf ) return;
	if( config_open( g_conf, "./qqconfig.txt" ) < 0 ){
		perror("can't not open qqconfig.txt file.");
		exit(-1); 
	}
	char* log_dir = config_readstr( g_conf, "QQLogDir" );
	uint log_terminal = config_readint( g_conf, "QQTerminalLog" );
	if( log_terminal ){
		debug_term_on();
	}else{
		debug_term_off();
	}
	if( log_dir == NULL ){
		debug_file_off();
	}else{
		debug_set_dir( log_dir );
		debug_file_on();
	}
}

void config_end()
{
	config_close( g_conf );
	DEL( g_conf );
}


//test
/*
int main()
{
	config* c = (config*)malloc( sizeof( config) );
	config_open( c, "./server.txt" );
	MSG("port: %d", config_readint( c, "ServerPort" ) );
	config_close( c );
	free( c );
	system("pause");
	return 0;
}
*/

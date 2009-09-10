/*
 *  utf8.c
 *
 *  utf8
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-10 13:31:57 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  utf8
 *
 */

#ifdef __WIN32__
#include <windows.h>
#else
#include <iconv.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "qqdef.h"
#include "debug.h"
#include "memory.h"
#include "config.h"
#include "utf8.h"



#ifdef __WIN32__
void utf8_to_gb ( char* src, char* dst, int len )
{
	int ret = 0;
	WCHAR* strA;
	int i= MultiByteToWideChar ( CP_UTF8, 0 , src, -1, NULL, 0 );
	if( i<=0 ){
		DBG("ERROR."); return;
	}
	strA = malloc( i*2 );
	MultiByteToWideChar ( CP_UTF8 , 0 , src, -1, strA , i);
	i= WideCharToMultiByte(CP_ACP,0,strA,-1,NULL,0,NULL,NULL);
	if( len >= i ){
		ret = WideCharToMultiByte (CP_ACP,0,strA,-1,dst,i,NULL,NULL);
		dst[i] = 0;
	}
	if( ret<=0 ){
		free( strA ); return;
	}
	free( strA );
}
void gb_to_utf8 ( char* src, char* dst, int len )
{
	int ret = 0;
	WCHAR* strA;
	int i= MultiByteToWideChar ( CP_ACP, 0 , src, -1, NULL, 0 );
	if( i<=0 ){
		DBG("ERROR."); return;
	}
	strA = malloc( i*2 );
	MultiByteToWideChar ( CP_ACP , 0 , src, -1, strA , i);
	i= WideCharToMultiByte(CP_UTF8,0,strA,-1,NULL,0,NULL,NULL);
	if( len >= i ){
		ret = WideCharToMultiByte (CP_UTF8,0,strA,-1,dst,i,NULL,NULL);
		dst[i] = 0;
	}
	if( ret<=0 ){
		free( strA ); return;
	}
	free( strA );
}
#else   //Linux
void utf8_to_gb( char* src, char* dst, int len )
{
	int ret = 0;
	uint inlen = strlen( src );
	uint outlen = len;
	char* inbuf = src;
	char* outbuf = dst;
	iconv_t cd;
	cd = iconv_open( "GBK", "UTF-8" );
	if ( cd != (iconv_t)-1 ){
		ret = iconv( cd, &inbuf, &inlen, &outbuf, &outlen );
		if( ret != 0 )
			printf("iconv failed err: %s\n", strerror(errno) );
		iconv_close( cd );
	}
}
void gb_to_utf8( char* src, char* dst, int len )
{
	int ret = 0;
	uint inlen = strlen( src );
	uint outlen = len;
	char* inbuf = src;
	char* outbuf = dst;
	iconv_t cd;
	cd = iconv_open( "UTF-8", "GBK" );
	if ( cd != (iconv_t)-1 ){
		ret = iconv( cd, &inbuf, &inlen, &outbuf, &outlen );
		if( ret != 0 )
			printf("iconv failed err: %s\n", strerror(errno) );
		iconv_close( cd );
	}
}
#endif


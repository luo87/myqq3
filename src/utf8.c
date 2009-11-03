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
// starkwong: In iconv implementations, inlen and outlen should be type of size_t not uint, which is different in length on Mac
void utf8_to_gb( char* src, char* dst, int len )
{
	int ret = 0;
	size_t inlen = strlen( src );
	size_t outlen = len;
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
	size_t inlen = strlen( src );
	size_t outlen = len;
	char* inbuf = src;
	char* outbuf2 = NULL;
	char* outbuf = dst;
	iconv_t cd;
	int c;

	// starkwong: if src==dst, the string will become invalid during conversion since UTF-8 is 3 chars in Chinese but GBK is mostly 2 chars
	if (src==dst) {
		outbuf2=(char*)malloc(len);
		memset(outbuf2,0,len);
		outbuf=outbuf2;
	}
	cd = iconv_open( "UTF-8", "GBK" );
	if ( cd != (iconv_t)-1 ){
		ret = iconv( cd, &inbuf, &inlen, &outbuf, &outlen );
		if( ret != 0 )
			printf("iconv failed err: %s\n", strerror(errno) );

		if (outbuf2!=NULL) {
			strcpy(dst,outbuf2);
			free(outbuf2);
		}

		iconv_close( cd );
	}
}
#endif


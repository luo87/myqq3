/*
 *  debug.c
 *
 *  MyQQ debugger.
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-01-31 Created.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#ifdef __WIN32__
#include <io.h>
#endif
#include <time.h>
#include "debug.h"
#include "util.h"
#include "qqdef.h"
#include "utf8.h"

static int dbg_term = 0, dbg_file = 0, log_day = 0;
static FILE* fp_log = NULL;
static char dir[128]={0,}, filename[160];
static void init_file_path();

void print_error(char* file, char* function, int line, const char *fmt, ...)
{
	va_list args;
	char printbuf[512];
	int i;
	if( !dbg_term && !dbg_file )
		return;
	va_start(args, fmt);
	i=vsnprintf( printbuf, 500, fmt, args );
	printbuf[i] = 0;
	va_end(args);
	if( dbg_term ){
		#ifdef __WIN32__
			utf8_to_gb( printbuf, printbuf, i );
		#endif
		printf("%s(%d): %s\n", function, line, printbuf);
	}
	if( dbg_file ){
		time_t t = time( NULL );
		struct tm* tm1 = localtime(&t);
		if( !tm1 ) return;
		if( tm1->tm_mday != log_day ){
			init_file_path();
		}
		char tmp[16];
		strftime( tmp, 15, "%X", tm1 );
		fprintf( fp_log, "%s [%s]%s(%d): %s\n", tmp, file, function, line, printbuf);
		fflush( fp_log );
	}
}

static char* hex_str(unsigned char *buf, int len, char* outstr )
{

	const char *set = "0123456789abcdef";
	char *tmp;
	unsigned char *end;
	if (len > 1024)
	len = 1024;
	end = buf + len;
	tmp = &outstr[0];
	while (buf < end)
	{
          *tmp++ = set[ (*buf) >> 4 ];
          *tmp++ = set[ (*buf) & 0xF ];
          *tmp++ = ' ';
          buf ++;
	}
	*tmp = '\0';
	return outstr;
}

void hex_dump( unsigned char * buf, int len )
{
	char str[KB(4)];
	if( dbg_term )
		puts( hex_str( buf, len, str ) );
	if( dbg_file ){
		fputs( hex_str( buf, len, str ), fp_log );
		fprintf( fp_log, "\n" );
		fflush( fp_log );
	}
	//fprintf( stderr, hex_str( buf, len ) );
}

void debug_term_on()
{
	dbg_term = 1;
}

void debug_term_off()
{
	dbg_term = 0;
}

void init_file_path()
{
	char tmp[64];
	time_t t = time( NULL );
	struct tm* tm1 = localtime(&t);
	if( !tm1 ){
		perror("log.c init_file_path: ERROR GETTING SYSTEM TIME.");
	}
	log_day = tm1->tm_mday;
	strftime( tmp, 64, "/%Y-%m-%d.txt", tm1 );
	if( access( dir, 0 )!=0 ){
		mkdir_recursive( dir );
	}
	strcpy( filename, dir );
	strcat( filename, tmp );
	if( fp_log )
		fclose( fp_log );
	fp_log = fopen( filename, "aw" );
//	fp_log = fopen( filename, "w" );
}

void debug_file_on()
{
	if( dbg_file )
		return;
	init_file_path();
	dbg_file = 1;
}

void debug_file_off()
{
	if( !dbg_file )
		return;
	dbg_file = 0;
	if( fp_log )
		fclose( fp_log );
}

void debug_set_dir(char* str)
{
	strcpy( dir, str );
}


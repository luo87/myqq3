/*
 *  crc32.c
 *
 *  CRC32
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  Hash
 *
 */

#include <stdio.h>
#include "qqdef.h"

static uint	CRC32[256];
static char	init = 0;

static void init_table()
{
	int  i,j;
	uint  crc;
	for(i = 0;i < 256;i++)
	{
		crc = i;
		for(j = 0;j < 8;j++)
		{
			if(crc & 1){
				crc = (crc >> 1) ^ 0xEDB88320;
			}else{
				crc = crc >> 1;
			}
		}
		CRC32[i] = crc;
	}
}

uint crc32( uchar *buf, int len)
{
	uint ret = 0xFFFFFFFF;
	int  i;
	if( !init ){
		init_table();
		init = 1;
	}
	for(i = 0; i < len;i++)
	{
		ret = CRC32[((ret & 0xFF) ^ buf[i])] ^ (ret >> 8);
	}
	ret = ~ret;
	return ret;
}


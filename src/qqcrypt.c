/**
 * The QQ2003C protocol plugin
 *
 * for gaim
 *
 * Copyright (C) 2004 Puzzlebird
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **************************************************
 * Reorganized by Minmin <csdengxm@hotmail.com>, 2005-3-27
 * Adapt it to MyQQ by Huang Guan <gdxxhg@gmail.com>, 2008-7-12
 **************************************************
 */

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "qqcrypt.h"
#include "debug.h"

static int random(void)
{
	/* it can be the real random seed function*/
	return 0xdead; /* override with number, convenient for debug*/	
}


static void encipher(unsigned int *const v, const unsigned int *const k, 
			unsigned int *const w)
{
	register unsigned int 
		y     = ntohl(v[0]),
		z     = ntohl(v[1]),
		a     = ntohl(k[0]),
		b     = ntohl(k[1]),
		c     = ntohl(k[2]),
		d     = ntohl(k[3]),
		n     = 0x10,       /* do encrypt 16 (0x10) times */
		sum   = 0,
		delta = 0x9E3779B9; /*  0x9E3779B9 - 0x100000000 = -0x61C88647 */

	while (n-- > 0) {
		sum += delta;
		y += ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
		z += ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
	}

	w[0] = htonl(y); w[1] = htonl(z);
}

static void decipher(unsigned int *const v, const unsigned int *const k, 
			unsigned int *const w)
{
	register unsigned int
		y     = ntohl(v[0]),
		z     = ntohl(v[1]),
		a     = ntohl(k[0]),
		b     = ntohl(k[1]),
		c     = ntohl(k[2]),
		d     = ntohl(k[3]),
		n     = 0x10,
		sum   = 0xE3779B90, 
		/* why this ? must be related with n value*/
		delta = 0x9E3779B9;

	/* sum = delta<<5, in general sum = delta * n */
	while (n-- > 0) {
		z -= ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
		y -= ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
		sum -= delta;
	}

	w[0] = htonl(y); w[1] = htonl(z);
}

void qqencrypt( unsigned char* instr, int instrlen, unsigned char* key,
			unsigned char*  outstr, int* outstrlen_ptr)
{
	unsigned char 
		plain[8],         /* plain text buffer*/
		plain_pre_8[8],   /* plain text buffer, previous 8 bytes*/
		* crypted,        /* crypted text*/
		* crypted_pre_8,  /* crypted test, previous 8 bytes*/
		* inp;            /* current position in instr*/
	int 
		pos_in_byte = 1,  /* loop in the byte */
		is_header=1,      /* header is one byte*/
		count=0,          /* number of bytes being crypted*/
		padding = 0;      /* number of padding stuff*/

	//void encrypt_every_8_byte (void);    

	/*** we encrypt every eight byte ***/
#define encrypt_every_8_byte()  \
	{\
	for(pos_in_byte=0; pos_in_byte<8; pos_in_byte++) {\
	if(is_header) { plain[pos_in_byte] ^= plain_pre_8[pos_in_byte]; }\
			else { plain[pos_in_byte] ^= crypted_pre_8[pos_in_byte]; }\
	} /* prepare plain text*/\
	encipher( (unsigned int *) plain,\
	(unsigned int *) key, \
	(unsigned int *) crypted);   /* encrypt it*/\
	\
	for(pos_in_byte=0; pos_in_byte<8; pos_in_byte++) {\
	crypted[pos_in_byte] ^= plain_pre_8[pos_in_byte]; \
	} \
	memcpy(plain_pre_8, plain, 8);     /* prepare next*/\
	\
	crypted_pre_8   =   crypted;       /* store position of previous 8 byte*/\
	crypted         +=  8;             /* prepare next output*/\
	count           +=  8;             /* outstrlen increase by 8*/\
	pos_in_byte     =   0;             /* back to start*/\
	is_header       =   0;             /* and exit header*/\
	}/* encrypt_every_8_byte*/

	pos_in_byte = (instrlen + 0x0a) % 8; /* header padding decided by instrlen*/
	if (pos_in_byte) { 
		pos_in_byte = 8 - pos_in_byte; 
	}
	plain[0] = (random() & 0xf8) | pos_in_byte;
	
	memset(plain+1, random()&0xff, pos_in_byte++);
	memset(plain_pre_8, 0x00, sizeof(plain_pre_8));

	crypted = crypted_pre_8 = outstr;

	padding = 1; /* pad some stuff in header*/
	while (padding <= 2) { /* at most two byte */
		if(pos_in_byte < 8) { plain[pos_in_byte++] = random() & 0xff; padding ++; } 
		if(pos_in_byte == 8){ encrypt_every_8_byte(); } 
	}

	inp = instr;
	while (instrlen > 0) {
		if (pos_in_byte < 8) { plain[pos_in_byte++] = *(inp++); instrlen --; }
		if (pos_in_byte == 8){ encrypt_every_8_byte(); }
	}

	padding = 1; /* pad some stuff in tailer*/
	while (padding <= 7) { /* at most sever byte*/
		if (pos_in_byte < 8) { plain[pos_in_byte++] = 0x00; padding ++; }
		if (pos_in_byte == 8){ encrypt_every_8_byte(); }
	} 

	*outstrlen_ptr = count;

}

int qqdecrypt( unsigned char* instr, int instrlen, unsigned char* key,
			unsigned char*  outstr, int* outstrlen_ptr)
{
	unsigned char 
		decrypted[8], m[8],
		* crypt_buff, 
		* crypt_buff_pre_8, 
		* outp;
	int 
		count, 
		context_start, 
		pos_in_byte, 
		padding;

#define decrypt_every_8_byte()  {\
	char bNeedRet = 0;\
	for (pos_in_byte = 0; pos_in_byte < 8; pos_in_byte ++ ) {\
	if (context_start + pos_in_byte >= instrlen) \
	{\
	bNeedRet = 1;\
	break;\
	}\
	decrypted[pos_in_byte] ^= crypt_buff[pos_in_byte];\
	}\
	if( !bNeedRet ) { \
	decipher( (unsigned int *) decrypted, \
	(unsigned int *) key, \
	(unsigned int *) decrypted);\
	\
	context_start +=  8;\
	crypt_buff    +=  8;\
	pos_in_byte   =   0;\
	}\
}/* decrypt_every_8_byte*/

	/* at least 16 bytes and %8 == 0*/
	if ((instrlen % 8) || (instrlen < 16)) return 0; 
	/* get information from header*/
	decipher( (unsigned int *) instr, 
		(unsigned int *) key, 
		(unsigned int *) decrypted);
	pos_in_byte = decrypted[0] & 0x7;
	count = instrlen - pos_in_byte - 10; /* this is the plaintext length*/
	/* return if outstr buffer is not large enought or error plaintext length*/
	if (*outstrlen_ptr < count || count < 0) return 0;

	memset(m, 0, 8);
	crypt_buff_pre_8 = m;
	*outstrlen_ptr = count;   /* everything is ok! set return string length*/

	crypt_buff = instr + 8;   /* address of real data start */
	context_start = 8;        /* context is at the second 8 byte*/
	pos_in_byte ++;           /* start of paddng stuffv*/

	padding = 1;              /* at least one in header*/
	while (padding <= 2) {    /* there are 2 byte padding stuff in header*/
		if (pos_in_byte < 8) {  /* bypass the padding stuff, none sense data*/
			pos_in_byte ++; padding ++;
		}
		if (pos_in_byte == 8) {
			crypt_buff_pre_8 = instr;
			//if (! decrypt_every_8_byte()) return 0; 
			decrypt_every_8_byte();
		}
	}/* while*/

	outp = outstr;
	while(count !=0) {
		if (pos_in_byte < 8) {
			*outp = crypt_buff_pre_8[pos_in_byte] ^ decrypted[pos_in_byte];
			outp ++;
			count --;
			pos_in_byte ++;
		}
		if (pos_in_byte == 8) {
			crypt_buff_pre_8 = crypt_buff - 8;
			//if (! decrypt_every_8_byte()) return 0;
			decrypt_every_8_byte();
		}
	}/* while*/
	for (padding = 1; padding < 8; padding ++) {
		if (pos_in_byte < 8) {
			if (crypt_buff_pre_8[pos_in_byte] ^ decrypted[pos_in_byte]) {
				return 0;
			}
			pos_in_byte ++; 
		}
		if (pos_in_byte == 8 ) {
			crypt_buff_pre_8 = crypt_buff;
			//if (! decrypt_every_8_byte()) return 0; 
			decrypt_every_8_byte();
		}
	}/* for*/
	return 1;

}

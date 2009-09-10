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

#ifndef _CRYPT_H
#define _CRYPT_H

int qqdecrypt( unsigned char* instr, int instrlen, unsigned char* key,
			unsigned char*  outstr, int* outstrlen_ptr);
void qqencrypt( unsigned char* instr, int instrlen, unsigned char* key,
			unsigned char*  outstr, int* outstrlen_ptr);

#endif //_CRYPT_H

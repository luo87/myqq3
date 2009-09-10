#ifndef _UTF8_H
#define _UTF8_H

void utf8_to_gb ( char* src, char* dst, int len );
void gb_to_utf8( char* src, char* dst, int len );
void decode_uri( char* input );
int if_UTF8(char *str);


#endif

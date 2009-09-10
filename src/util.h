#ifndef _UTIL_H
#define _UTIL_H

int mkdir_recursive( char* path );
int trans_faces( char* src, char* dst, int outlen );
int http_request( int* http_sock, char* url, char* session, char* data, int* datalen );
char* mid_value( char* str, char* left, char* right, char* out, int outlen );
void msleep( unsigned int ms ); 
int get_splitable_pos( char* buf, int pos );

#endif


#ifndef _WEBQQ_H
#define _WEBQQ_H

#include <time.h>

typedef struct user{
	time_t	update_time;	//if now-update_time
	time_t	create_time;	//
	void* qq;
	void*	session_ptr;	//identical
	int	reference;
}user;


#ifdef __WIN32__

#define STDCALL __stdcall
#ifdef WEBQQLIB
#define EXPORT __declspec(dllexport) STDCALL 
#else
#define EXPORT __declspec(dllimport) STDCALL
#endif

#else
#define EXPORT 
#endif


EXPORT void webqq_init();
EXPORT void webqq_end();
EXPORT user* webqq_get_user( void* session_ptr );
EXPORT user* webqq_create_user( void* session_ptr, uint number, uchar* md5pass );
EXPORT void webqq_derefer( user* u );
EXPORT int webqq_login( user* u );
EXPORT int webqq_logout( user* u );
EXPORT int webqq_recv_msg( user* u, char* buf, int size, int wait );
EXPORT int webqq_send_msg( user* u, uint to, char* buf, char qun_msg );
EXPORT void webqq_update_list( user* u );
EXPORT void webqq_verify( user* u, uint code );
EXPORT void webqq_remove( user* u );
EXPORT void webqq_status( user* u, int st );

#endif

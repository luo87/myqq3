#ifndef _libQQ_H
#define _libQQ_H

#include <time.h>

#ifdef __WIN32__

#define STDCALL __stdcall
#ifdef LIBQQLIB
#define EXPORT __declspec(dllexport) STDCALL 
#else
#define EXPORT __declspec(dllimport) STDCALL
#endif

#else
#define EXPORT 
#endif

struct qqclient;

EXPORT void libqq_init();
EXPORT void libqq_cleanup();
EXPORT struct qqclient* libqq_create( uint number, char* pass );
EXPORT int libqq_login( struct qqclient* qq );
EXPORT int libqq_logout( struct qqclient* qq );
EXPORT int libqq_keepalive( struct qqclient* qq );
EXPORT int libqq_detach( struct qqclient* qq );
EXPORT int libqq_getmessage( struct qqclient* qq, char* buf, int size, int wait );
EXPORT int libqq_sendmessage( struct qqclient* qq, uint to, char* buf, char qun_msg );
EXPORT void libqq_updatelist( struct qqclient* qq );
EXPORT void libqq_verify( struct qqclient* qq, const char* code );
EXPORT void libqq_remove( struct qqclient* qq );
EXPORT void libqq_status( struct qqclient* qq, int st, uchar has_camera );
EXPORT uint libqq_refresh( struct qqclient* qq );
EXPORT void libqq_getqunname( struct qqclient* qq, uint ext_id, char* buf );
EXPORT void libqq_getqunmembername( struct qqclient* qq, uint ext_id, uint uid, char* buf );
EXPORT void libqq_getbuddyname( struct qqclient* qq, uint uid, char* buf );
EXPORT void libqq_getextrainfo( struct qqclient* qq, uint uid );
EXPORT void libqq_addbuddy( struct qqclient* qq, uint uid, char* msg );
EXPORT void libqq_delbuddy( struct qqclient* qq, uint uid );
EXPORT void libqq_sethttpproxy( struct qqclient* qq, char* ip, ushort port );

#endif

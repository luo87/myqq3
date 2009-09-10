#ifndef _GROUP_H
#define _GROUP_H

#include "qqdef.h"

typedef struct qqgroup{
	uint 		number;		//internal number
	char		name[GROUPNAME_LEN];
}qqgroup;

struct qqclient;
qqgroup* group_get( struct qqclient* qq, uint number, int create );
void group_remove( struct qqclient* qq, uint number );
void group_update_list( struct qqclient* qq );
void group_update_info( struct qqclient* qq, qqgroup* g );
void group_put_event( struct qqclient* qq );

#endif


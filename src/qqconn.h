#ifndef QQ_CONN_H_
#define QQ_CONN_H_


int qqconn_connect( qqclient* qq );
void qqconn_load_serverlist();
void qqconn_get_server(qqclient* qq);
int qqconn_establish( qqclient* qq );

#endif //QQ_CONN_H_

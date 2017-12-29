#ifndef __CLIENT_H__
#define __CLIENT_H__

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

int msg_thread_init(void *addr);
int receive_from_server(char *buf);
int send_to_server(char *buf, int length);

#endif


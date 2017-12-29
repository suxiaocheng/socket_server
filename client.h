#ifndef __CLIENT_H__
#define __CLIENT_H__

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/**
 * msg_thread_init - init two background thread to send and receive data.
 * @addr:	internet address to connect.
 * @return:	0:suecess, 1: fail
 * Description: Used to connect to socket.
 */
int msg_thread_init(void *addr);

/**
 * receive_from_server - used to receive msg from server. Buf size
 * is at least 2048
 * @buf:	
 * @return:	-1: fail, >0: number of bytes get from server.
 * Description: Used to connect to socket.
 */
int receive_from_server(char *buf);
/**
 * send_to_server - send data to server
 * @buf:	buffer used to send data
 * @length:	buffer length, should not exceed 2044
 * @return:	0:suecess, -1: fail
 */
int send_to_server(char *buf, int length);

#endif


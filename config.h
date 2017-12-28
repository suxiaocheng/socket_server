/*
 * config.h 包含该tcp/ip套接字编程所需要的基本头文件，与server.c client.c位于同一目录下
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "debug.h"

const int MAX_LINE = 2048;
const int PORT = 6011;
const int BACKLOG = 10;
const int LISTENQ = 6666;
const int MAX_CONNECT = 20;

typedef struct{
	pid_t pid;
	int msg;
	void *next;
	int connfd;
}child_info_t;

typedef void (*sighandler_t)(int);
typedef int (*test_function_t)(void);

#define quit_sequence   "quit"
#define msg_sequence	"msg "



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "client.h"

int main(int argc, char **argv)
{
	char buf[1024];
	ssize_t length;
	int need_quit = FALSE;
	int ret;
	
	/* wait for socket connection */
	while(1) {
		ret = msg_thread_init("127.0.0.1");
		if(ret == 0){
			break;
		}
		printf("Socket connect fail, retry\n");
		sleep(1);
	}

	while(1){
		printf("Please input a string\n");
		length = read(STDIN_FILENO, buf, sizeof(buf));
		buf[length] = 0;
		switch(buf[0]){
			case 'q':
			need_quit = TRUE;
			break;
			case 'w':
			send_to_server(&buf[1], length-1);
			break;
			case 'r':
			length = receive_from_server(buf);
			if(length > 0) {
				buf[length] = 0;
				printf("Receive buf is: %s\n", buf);
			}else {
				printf("length: %d\n", (int)length);
			}
			break;
			default:
			printf("%s\n", buf);
			break;
		}
		if(need_quit == TRUE){
			break;
		}
	}

	return 0;
}


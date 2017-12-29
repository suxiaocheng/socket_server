/*
 * client.c为客户端代码
*/

#include "config.h"
pid_t send_pid, recv_pid;

int msg_thread_init(void *addr)
{
	int sockfd;
	struct sockaddr_in servaddr;
	int ret;
	char dummy = 0;

	/*(1) 创建套接字*/
	if((sockfd = socket(AF_INET , SOCK_STREAM , 0)) == -1)
	{
		printf("socket error");
		return 1;
	}

	/*(2) 设置链接服务器地址结构*/
	bzero(&servaddr , sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	if(inet_pton(AF_INET , addr, &servaddr.sin_addr) < 0)
	{
		printf("inet_pton error for %s\n", (char *)addr);
		return 1;
	}

	/*(3) 发送链接服务器请求*/
	if( connect(sockfd , (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0)
	{
		printf("connect error");
		return 1;
	}

	/*(4) 消息处理*/
	char sendline[MAX_LINE] , recvline[MAX_LINE];
	send_pid = fork();
	if(send_pid == 0) {
		int msgid = msgget(getpid(), IPC_CREAT|0666);
		memcpy(sendline, msg_sequence, sizeof(msg_sequence) - 1);
		while(1){
			ret = msgrcv(msgid, &sendline[sizeof(msg_sequence)-1], 
				MAX_LINE - (sizeof(msg_sequence)-1), 0, IPC_NOWAIT);
			if(ret > 0){
				debug("pid%d: ready to send data\n", getpid());
				dump_memory(sendline, ret + sizeof(msg_sequence) - 1);
				if(ret == 1){
					break;
				}
				write(sockfd , sendline , ret + sizeof(msg_sequence) - 1);
			} else if((ret == -1) && (errno != ENOMSG)) {
				err("pid%d, errno: %d\n", getpid(), errno);
			}
		}
		ret = msgctl(msgid, IPC_RMID, 0);
		if (ret == -1) {
			err("child%d, msg queue fail\n", getpid());
		}
		debug("child%d, quit\n", getpid());
	} else {
		recv_pid = fork();
		if(recv_pid == 0) {
			int msgid = msgget(getpid(), IPC_CREAT|0666);
			if(msgid == -1) {
				err("pid%d, create receive msg queue fail\n", getpid());
			}
			int send_msgid = msgget(send_pid, IPC_CREAT|0666);
			if(send_msgid == -1) {
				err("pid%d, create send msg queue fail\n", getpid());
			}
			while(1){
				ret = read(sockfd, recvline, MAX_LINE);
				if(ret != -1){
					debug("pid%d: recv data from server:\n", getpid());
					dump_memory(recvline, ret);
					if(strncmp(quit_sequence, recvline, sizeof(quit_sequence - 1)) == 0){
						debug("pid%d: Receive quit msg\n", getpid());
						/* Resend to server */
						write(sockfd , quit_sequence , sizeof(quit_sequence));
						/* Force send process quit */
						ret = msgsnd(send_msgid, &dummy, 1, 0);
						if(ret == -1){
							debug("Quit send thread fail\n");
						}
						break;
					} else {
						ret = msgsnd(msgid, &recvline[sizeof(msg_sequence)-1], 
							ret - (sizeof(msg_sequence)-1), 0);
						if(ret == -1) {
							err("pid%d: send msg fail, errno: %d\n", getpid(), errno);
						}
					}
				} else {
					err("read socket err\n");
				}
			}
			ret = msgctl(msgid, IPC_RMID, 0);
			if (ret == -1) {
				err("child%d, msg queue fail\n", getpid());
			}
			debug("child%d, quit\n", getpid());
		}
	}
	
	/*(5) 关闭套接字*/
	close(sockfd);
	return 0;
}

int receive_from_server(char *buf)
{
	int ret;
	int msgid = msgget(recv_pid, IPC_CREAT|0666);
	while(1) {
		ret = msgrcv(msgid, buf, MAX_LINE - (sizeof(msg_sequence)-1), 0, IPC_NOWAIT);
		if((ret > 0) || ((ret == -1) && (errno == ENOMSG))) {
			break;
		}
	}
	
	return ret;
}

int send_to_server(char *buf, int length)
{
	int ret;
	int msgid = msgget(send_pid, IPC_CREAT|0666);
	ret = msgsnd(msgid, buf, length, 0);

	return ret;
}



/*
 * server.c为服务器端代码
*/

#include "config.h"

child_info_t *child_info_head = NULL;
int connfd = -1;
int listenfd = -1;
pid_t parent_pid = -1;
pid_t broadcastpid = -1;

void add_child_info(child_info_t *child)
{
	child->next = child_info_head;
	child_info_head = child;
}

int remove_child_info(child_info_t *child)
{
	int ret = 0;
	child_info_t *item;

	if (child == child_info_head) {
		child_info_head = child->next;
		return 0;
	}

	for (item = child_info_head; item != NULL; item=item->next) {
		if (item->next == child) {
			item->next = ((child_info_t *)(item->next))->next;
			return 0;
		}
	}
	return 1;
}

int quit_broadcast_thread(pid_t broadcastpid)
{
	int ret;
	int flag = 1;
	if (broadcastpid != (pid_t)-1) {
		int msgid = msgget(broadcastpid, IPC_CREAT|0666);
		if(msgid == -1){
			err("pid%d, waitpid create msg queue fail\n", getpid());
		} else {
			ret = msgsnd(msgid, quit_sequence, sizeof(quit_sequence), 0) ;
			if(ret == -1){
				err("pid%d, msgsnd fail\n", getpid());
			} else {
				ret = waitpid(broadcastpid, NULL, 0);
				if (ret != broadcastpid){
					err("pid%d, Wait %d exit fail\n", getpid(), ret);
				} else {
					debug("pid%d, broadcast thread exit ok\n", getpid());
					flag = 0;
				}
			}
		}
		ret = msgctl(msgid, IPC_RMID, 0);
		if (ret == -1) {
			err("pid%d, Remove broadcast msg queue fail\n", getpid());
		}
		broadcastpid = -1;
	}
	return flag;
}

int quit_child_thread(void)
{
	child_info_t *pchild_info;
	child_info_t *pchild_info_next;
	int ret;

	debug("pid%d, going to quit child thread\n", getpid());

	for(pchild_info = child_info_head; pchild_info != NULL; pchild_info = pchild_info_next) {
		if(pchild_info != NULL){
			pchild_info_next = pchild_info->next;
		}

		debug("pid%d, Child pid: %d\n", getpid(), pchild_info->pid);

		if(pchild_info->connfd != -1) {
			ret = write(pchild_info->connfd, quit_sequence, sizeof(quit_sequence));
			if(ret == sizeof(quit_sequence)){
				debug("pid%d, wait process %d to exit\n", getpid(), pchild_info->pid);
				ret = waitpid(pchild_info->pid, NULL, 0);
				if (ret != pchild_info->pid){
					err("pid%d, wait process %d exit fail\n", getpid(), pchild_info->pid);
				} else {
					debug("pid%d, thread %d exit ok\n", getpid(), pchild_info->pid);
				}
			}
			close(pchild_info->connfd);
			pchild_info->connfd = -1;
		}
		free(pchild_info);
	}
	debug("pid%d, finish\n", getpid());

	child_info_head = NULL;
	return 0;
}

sighandler_t signal_handler(void)
{
	int err;

	debug("pid%d, Receive interrupt or term signal\n", getpid());	

	if(parent_pid != getpid()) {
		debug("pid%d, not parent, return\n", getpid());
		return 0;
	}
		
	quit_child_thread();

	quit_broadcast_thread(broadcastpid);

	if(listenfd != -1){
		debug("pid%d, wait thread exit\n", getpid());
		close(listenfd);
		listenfd = -1;
	}

	close_log_file();

	_exit(-1);
	return NULL;
}

int main(int argc , char **argv)
{
	struct sockaddr_in servaddr , cliaddr;
	pid_t childpid;
	char buf[MAX_LINE];
	socklen_t clilen;
	child_info_t *pchild_info;
	child_info_t *pchild_info_next;
	int i;
	int ret;
	int reuse = 1;

	signal((int) SIGINT, (sighandler_t) signal_handler); /* handle user interrupt */
    	signal((int) SIGTERM, (sighandler_t) signal_handler);	/* handle kill from shell */

	init_log_file("/tmp/socket.log");

	parent_pid = getpid();
	
	/*(1) 初始化监听套接字listenfd*/
	if((listenfd = socket(AF_INET , SOCK_STREAM , 0)) < 0)
	{
		perror("socket error");
		exit(1);
	}
	
	/*(2) 设置服务器sockaddr_in结构*/
	bzero(&servaddr , sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(ret == -1){
		perror("set socket reuse fail\n");
	}

	/*(3) 绑定套接字和端口*/
	if(bind(listenfd , (struct sockaddr*)&servaddr , sizeof(servaddr)) < 0)
	{
		perror("bind error");
		exit(1);
	}

	/*(4) 监听客户请求*/
	if(listen(listenfd , LISTENQ) < 0)
	{
		perror("listen error");
		exit(1);
	}

	/*(5) 接受客户请求*/
	for( ; ; )
	{
		clilen = sizeof(cliaddr);
		if((connfd = accept(listenfd , (struct sockaddr *)&cliaddr , &clilen)) < 0 )
		{
			perror("accept error");
			exit(1);
		}

		if((childpid = fork()) == 0) 
		{
			close(listenfd);
			//str_echo
			ssize_t n;
			char buff[MAX_LINE];
			int msgid = msgget(getpid(), IPC_CREAT|0666);
			if(msgid == -1){
				err("child%d, create msg queue fail\n", getpid());
			}

			debug("child%d, Fork child ok\n", getpid());

			for(pchild_info = child_info_head; pchild_info != NULL; pchild_info = pchild_info_next) {
				pchild_info_next = pchild_info->next;
				ret = close(pchild_info->connfd);
				if (ret == -1) {
					err("child%d, Close connfd pid[%d] fail\n", getpid(), pchild_info->pid);
				}
				free(pchild_info);
			}
			child_info_head = NULL;

			while((n = read(connfd , buff , MAX_LINE)) > 0)
			{
				if(msgid != -1) {
					debug("child%d, receive msg:\n", getpid());
					dump_memory(buff, n);
					if(strncmp(buff, quit_sequence, sizeof(quit_sequence) - 1) == 0){
						break;
					} else {
						/* verify the msg */
						if(strncmp(buff, msg_sequence, sizeof(msg_sequence) - 1) != 0){
							err("child%d, receive msg not start with %s\n", getpid(), msg_sequence);
						}
						ret = msgsnd(msgid, &buff, n, 0) ;
						if(ret == -1){
							err("child%d, msgsnd fail\n", getpid());
						}
					}
				}
			}
			ret = msgctl(msgid, IPC_RMID, 0);
			if (ret == -1) {
				err("child%d, msg queue fail\n", getpid());
			}
			debug("child%d, exit\n", getpid());
			close(connfd);
			return 0;
		} else {
			pchild_info = (child_info_t *)malloc(sizeof(child_info_t));
			pchild_info->pid = childpid;
			pchild_info->connfd = connfd;

			add_child_info(pchild_info);
			quit_broadcast_thread(broadcastpid);

			broadcastpid = fork();
			if (broadcastpid == 0) {
				close(listenfd);
				int msgid = msgget(getpid(), IPC_CREAT|0666);
				int matchid;
				for(pchild_info = child_info_head; pchild_info != NULL; pchild_info = pchild_info->next) {
					pchild_info->msg = msgget(pchild_info->pid, IPC_CREAT|0666);
					if (pchild_info->msg == -1) {
						err("broadcast%d, Create msg queue for pid%d fail\n", getpid(), pchild_info->pid);
					}
				}
				debug("broadcast%d, is running\n", getpid());
				while(1){
					matchid = -1;
					for(pchild_info = child_info_head; pchild_info != NULL; pchild_info = pchild_info->next) {
						if(pchild_info->msg != -1) {
							ret = msgrcv(pchild_info->msg, &buf, MAX_LINE, 0, IPC_NOWAIT);
							if((ret == -1) && (errno != ENOMSG)) {
								if(errno == EINVAL) {
									/* child process has exit */
									pchild_info->msg = -1;
								}else {
									err("broadcast%d, msgrcv receive from pid%d error: %d\n", getpid(), pchild_info->pid, errno);
								}
							} else if(ret > 0){
								/* Get msg */
								matchid = pchild_info->pid;
								debug("broadcast%d, msgrcv get msg from pid: %d\n", getpid(), pchild_info->pid);
								break;
							}
						}
					}
					if(pchild_info!= NULL) {
						for(pchild_info = child_info_head; pchild_info != NULL; pchild_info = pchild_info->next) {
							if (matchid == pchild_info->pid) {
								continue;
							}
							ret = write(pchild_info->connfd, buf, ret);
							debug("broadcast%d, write pid %d data %d bytes\n", getpid(), pchild_info->pid, ret);
							if(ret > 0) {
								dump_memory(buf, ret);
							}
						}
					}
					/* check if thread need to exit */
					{						
						ret = msgrcv(msgid, &buf, MAX_LINE, 0, IPC_NOWAIT);
						if(ret > 0){
							debug("broadcast%d, Broadcast thread receive msg: %s\n", getpid(), buf);
							break;
						}
					}
					usleep(1000);
				}
				debug("broadcast%d, thread quit\n", getpid());
				return 0;
			}
		}
	}

	quit_child_thread();

	quit_broadcast_thread(broadcastpid);	

	debug("main%d, exit\n", getpid());
	
	/*(6) 关闭监听套接字*/
	if(listenfd != -1) {
		close(listenfd);
		listenfd = -1;
	}
	return 0;
}


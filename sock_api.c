#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>		/* memset() */
#include <pthread.h>

#define CLR(x) memset(&x, 0, sizeof(x))
#define IF_RET(x) if(-1==(x)) return -1

/**
 * create_srv
 * @port: the bind port in use
 * @backlog: maximum connection counts
 *
 * Short description of this function
 *
 * @Returns: return the fd when success
 */
int create_srv(int port, int backlog)
{
	struct sockaddr_in addr;
	int fd;

	IF_RET(fd = socket(AF_INET, SOCK_STREAM, 0));

	CLR(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	IF_RET(bind(fd, (struct sockaddr *) &addr, sizeof(addr)));
	IF_RET(listen(fd, backlog));

	
	return fd;
}


void close_srv(int srv_fd)
{
	close(srv_fd);
}


void log_conn(struct sockaddr_in addr)
{
	/** TODO **/
	printf("conn from %s\n", inet_ntoa(addr.sin_addr));
	
}

void *send_data(void *fd)
{
	/** TODO **/
	send(*((int *)fd),"Welcome to my server/n",21,0);
	pthread_exit(NULL);
}

int loop_srv(int srv_fd)
{
	int conn_fd;
	struct sockaddr_in addr;
	int conn_addr_len;
	pthread_t pid;

	conn_addr_len = sizeof(struct sockaddr_in);
	while(1)
	{
		if(-1 == (conn_fd = accept(srv_fd, (struct sockaddr*) &addr, (socklen_t *) &conn_addr_len)))
		{
			perror("accept");
			continue;
		}
		log_conn(addr);
		pthread_create(&pid,NULL, send_data, &conn_fd);
		close(conn_fd);
	}

	return 0;
}


/**
 * function description
 * @ip: 111.222.333.444
 * @port: port
 * @Returns: fd
**/
int create_clnt(char *ip, int port)
{
	struct sockaddr_in addr;
	int fd;

	IF_RET(fd = socket(AF_INET, SOCK_STREAM, 0));

	CLR(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	inet_pton(AF_INET,ip,&addr.sin_addr);

	IF_RET(connect(fd, (struct sockaddr *) &addr, sizeof(addr)));	
	return fd;
}

int loop_clnt(int clnt_fd)
{
	/** TODO **/
	return 0;
}
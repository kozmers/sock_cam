#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8888
#define BACK_LOG 1024

void *s_listen(void *fd)
{
	loop_srv(*((int *)fd));
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	pthread_t pid;
	int c_fd;
	int s_fd = create_srv(PORT,BACK_LOG);

	pthread_create(&pid,NULL,s_listen, &s_fd);
 	
	sleep(3);

	c_fd = create_clnt("127.0.0.1", PORT);
	loop_clnt(c_fd);
	
	close_srv(s_fd);
	close_clnt(c_fd);
	

	return 0;
}
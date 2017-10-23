#include<unistd.h>
#include<time.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<sys/sendfile.h>
#include<errno.h>

#include<vector>
#include<algorithm>

#include"connection.h"
#include"net.h"
#include"util.h"

extern vector<Connection*> connections;
extern int epfd;

bool cmp(Connection* c1, Connection* c2)
{
	return c1->active_time > c2->active_time;
}

int accept_connection(void* listen_fd)
{
	int fd = *((int*)listen_fd);

	int connfd;
	while((connfd = accept(fd, NULL, NULL)) != -1)
	{
		if(connections.size() >= MAX_CONNECTIONS)
		{
			close(connfd);
			continue;
		}
		if(set_nonblock(connfd) == ABYSS_ERR)
		{
			close(connfd);
			continue;
		}
		Connection* c = new Connection();
		c->connfd = connfd;
		c->event.events = EPOLLIN;
		c->event.data.ptr->ptr = c;
		c->event.data.ptr->in_handler = recv_request;
		c->event.data.ptr->out_handler = send_response;
		if(epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &c->event) == ABYSS_ERR)
		{
			close(connfd);
			delete c;
			continue;
		}
		c->activae_time = time(NULL);
		connections.push_back(c);
		push_heap(connections.begin(), connections.end(), cmp);
	}

	if(errno != EWOULDBLOCK)
	{
		ABYSS_ERR_MSG(strerror(errno));
		return ABYSS_ERR;
	}
	return ABYSS_OK;
}

int close_connection(void* connection)
{
	Connection* c = connection;
	
}
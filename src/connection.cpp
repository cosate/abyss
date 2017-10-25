#include<unistd.h>
#include<time.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<sys/sendfile.h>
#include<sys/types.h>
#include<errno.h>
#include<cstring>

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
		c->event.data.ptr->in_handler = handle_request;
		c->event.data.ptr->out_handler = handle_response;
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

int handle_request(void* connection)
{
	Connection* c = connection;
	recv_request(c);
}

int handle_response(void* connection)
{
	Connection* c = connection;
	
}

static int close_connection(void* connection)
{
	Connection* c = connection;
	
}

static int recv_request(Connection* c)
{
	while(1)
	{
		ssize_t bytes = recv(c->connfd, c->recv_buffer + c->buffer_length, BUFFER_SIZE - c->buffer_length, 0);
		if(bytes == 0)
			return ABYSS_ERR;
		else if(bytes == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				return ABYSS_OK;
			else
				return ABYSS_ERR;
		}
		else
		{
			c->buffer_length += bytes;
			if(BUFFER_SIZE == c->buffer_length)
				return ABYSS_OK;
		}
	}

}

static int send_response(Connection* c)
{
	
}
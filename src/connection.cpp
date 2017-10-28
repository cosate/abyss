#include<unistd.h>
#include<time.h>
#include<sys/socket.h>
#include<sys/epoll.h>
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
#include"message.h"

extern vector<EventData*> connections;
extern int epfd;

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

bool cmp(EventData* c1, EventData* c2)
{
	return c1->active_time > c2->active_time;
}

void ConnectionData::construct()
{
	this->request = Request();
	this->response = Response();
	memset(this->send_buffer, 0, BUFFERSIZE);
	memset(this->recv_buffer, 0, BUFFERSIZE);
	this->buffer_length = 0;
}

int ConnectionData::in_handler()
{
	
}

int ConnectionData::out_handler()
{

}

int ListenData::in_handler()
{
	int connfd;
	while((connfd = accept(this->fd, NULL, NULL)) != -1)
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
		EventData* c = new ConnectionData(connfd);
		
		epoll_event connev;
		connev.events = EPOLLIN;
		connev.data.ptr = c;
		
		if(epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &connev) == ABYSS_ERR)
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
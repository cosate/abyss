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

bool cmp(EventData* c1, EventData* c2)
{
	return c1->active_time > c2->active_time;
}

int ConnectionData::enable_in()
{
	if(!(this->events & EPOLLIN))
	{
		this->events |= EPOLLIN;

		epoll_event ev;
		ev.events = this->events;
		ev.data.ptr = (EventData*)this;
		if(epoll_ctl(epfd, EPOLL_CTL_MOD, this->fd, &ev) == -1)
		{
			ABYSS_ERR_MSG(strerror(errno));
			return ABYSS_ERR;
		}
	}
	return ABYSS_OK;
}

int ConnectionData::disable_in()
{
	if(this->events & EPOLLIN)
	{
		this->events ^= EPOLLIN;

		epoll_event ev;
		ev.events = this->events;
		ev.data.ptr = (EventData*)this;
		if(epoll_ctl(epfd, EPOLL_CTL_MOD, this->fd, &ev) == -1)
		{
			ABYSS_ERR_MSG(strerror(errno));
			return ABYSS_ERR;
		}
	}
	return ABYSS_OK;
}

int ConnectionData::enable_out()
{
	if(!(this->events & EPOLLOUT))
	{
		this->events |= EPOLLOUT;

		epoll_event ev;
		ev.events = this->events;
		ev.data.ptr = (EventData*)this;
		if(epoll_ctl(epfd, EPOLL_CTL_MOD, this->fd, &ev) == -1)
		{
			ABYSS_ERR_MSG(strerror(errno));
			return ABYSS_ERR;
		}
	}
	return ABYSS_OK;
}

int ConnectionData::disable_out()
{
	if(this->events & EPOLLOUT)
	{
		this->events ^= EPOLLIN;

		epoll_event ev;
		ev.events = this->events;
		ev.data.ptr = (EventData*)this;
		if(epoll_ctl(epfd, EPOLL_CTL_MOD, this->fd, &ev) == -1)
		{
			ABYSS_ERR_MSG(strerror(errno));
			return ABYSS_ERR;
		}
	}
	return ABYSS_OK;
}

int ConnectionData::recv_request()
{
	while(1)
	{
		ssize_t bytes = recv(this->fd, this->recv_buffer + this->buffer_length, BUFFER_SIZE - this->buffer_length, 0);
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
			this->buffer_length += bytes;
			if(BUFFER_SIZE == c->buffer_length)
				return ABYSS_OK;
		}
	}
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
	if(this->recv_request() == ABYSS_ERR)
	{
		return ABYSS_ERR;
	}
	if(parse_request() == ABYSS_ERR)
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
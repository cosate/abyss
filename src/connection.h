#ifndef ABYSS_CONNECTION_H
#define ABYSS_CONNECTION_H

#include<sys/epoll.h>
#include<sys/types.h>
#include"message.h"

#define BUFFERSIZE (4096)

typedef int (*handler)(void*);

class EventData
{
public:
	void* ptr;
	handler in_handler;
	handler out_handler;
	EventData():ptr(NULL), in_handler(NULL), out_handler(NULL){}
};

class Connection
{
public:
	int connfd;
	epoll_event event;
	time_t active_time;
	Request request;
	Response response;
	char send_buf[BUFFERSIZE];
	char recv_buf[BUFFERSIZE];
	Connection()
	{
		connfd = 0;
		event.events = 0;
		event.data.ptr = new EventData();
		active_time = 0;
		request = Request();
		response = Response();
		memset(send_buf, 0, BUFFERSIZE);
		memset(recv_buf, 0, BUFFERSIZE);
	}
};

int accept_connection(void*);
int close_connection(void*);
int recv_request(void*);
int send_response(void*);

#endif
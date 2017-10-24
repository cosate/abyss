#ifndef ABYSS_CONNECTION_H
#define ABYSS_CONNECTION_H

#include<sys/epoll.h>
#include<sys/types.h>
#include"message.h"

#define BUFFER_SIZE (4096)
#define MAX_CONNECTIONS (10000)
#define MAX_EVENTs (10000)

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
	char send_buffer[BUFFERSIZE];
	char recv_buffer[BUFFERSIZE];
	int buffer_length;
	Connection()
	{
		connfd = 0;
		event.events = 0;
		event.data.ptr = new EventData();
		active_time = 0;
		request = Request();
		response = Response();
		memset(send_buffer, 0, BUFFERSIZE);
		memset(recv_buffer, 0, BUFFERSIZE);
		buffer_length = 0;
	}

	~Connection()
	{
		delete event.data.ptr;
	}
};

bool cmp(Connection*, Connection*);

int accept_connection(void*);
int close_connection(void*);
int handle_request(void*);
int handle_response(void*);

#endif
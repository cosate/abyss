#ifndef ABYSS_CONNECTION_H
#define ABYSS_CONNECTION_H

#include<sys/types.h>
#include<sys/epoll.h>
#include"message.h"

#define BUFFER_SIZE (4096)
#define MAX_CONNECTIONS (10000)
#define MAX_EVENTs (10000)

class EventData
{
public:
	int fd;
	uint32_t events;
	virtual int in_handler() = 0;
	EventData() : fd(-1), events(EPOLLIN) {}
	EventData(int f) :fd(f), events(EPOLLIN) {}
};

class ConnectionData : public EventData
{
public:
	time_t active_time;
	Request request;
	Response response;
	char send_buffer[BUFFERSIZE];
	char recv_buffer[BUFFERSIZE];
	int buffer_length;
	Connection() : EventData()
	{
		construct();
	}

	Connection(int f) : EventData(f)
	{
		construct();
	}

	int in_handler();
	int out_handler();
private:
	struct parse_status
	{
		char* line_begin;
		char* line_end;
		char* current;
	};
	void construct();

	int enable_in();
	int disable_in();
	int enable_out();
	int disable_out();

	int recv_request();
	int parse_request();
};

class ListenData : public EventData
{
public:
	ListenData() : EventData() {}
	ListenData(int f) : EventData(f) {}

	int in_handler();
};

bool cmp(EventData*, EventData*);

#endif
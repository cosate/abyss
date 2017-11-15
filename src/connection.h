#ifndef ABYSS_CONNECTION_H
#define ABYSS_CONNECTION_H

#include<sys/types.h>
#include<sys/epoll.h>
#include"message.h"

#define BUFFER_SIZE (4096)
#define MAX_CONNECTIONS (10000)
#define MAX_EVENTS (10000)

class EventData
{
public:
	int fd;
	uint32_t events;
	EventData() : fd(-1), events(EPOLLIN) {}
	EventData(int f) :fd(f), events(EPOLLIN) {}

	virtual int in_handler();
	virtual int out_handler();

	virtual ~EventData() = 0;
};

class ListenData : public EventData
{
public:
	ListenData() : EventData() {}
	ListenData(int f) : EventData(f) {}

	~ListenData(){}

	int in_handler();
};

enum class Parse_Stage
{
	PARSE_REQUEST_LINE = 0,
	PARSE_METHOD,
    PARSE_URL,
    PARSE_URL_SCHEME,
    PARSE_URL_HOST,
    PARSE_URL_PORT,
    PARSE_URL_PATH,
    PARSE_URL_QUERY,
    PARSE_HTTP_VERSION,

    PARSE_HEADER,
    PARSE_HEADER_NAME,
    PARSE_HEADER_VALUE,

    PARSE_BODY,
    PARSE_DONE
};

class ConnectionData : public EventData
{
public:
	time_t active_time;
	Request request;
	Response response;
	char send_buffer[BUFFER_SIZE];
	char recv_buffer[BUFFER_SIZE];
	int recv_buffer_length;
	int send_buffer_length;

	struct Parse_Status
	{
		char* section_begin;
		char* current;
		Parse_Stage stage;
	}parse_status;

	struct Send_Status
	{
		char* send_begin;
	}send_status;

	ConnectionData() : EventData()
	{
		construct();
	}

	ConnectionData(int f) : EventData(f)
	{
		construct();
	}

	~ConnectionData(){}

	int in_handler();
	int out_handler();
private:
	void construct();

	int enable_in();
	int disable_in();
	int enable_out();
	int disable_out();

	int recv_request();
	int parse_request();

	void pass_whitespace();
	bool is_valid_scheme_char(char);
	bool is_valid_host_char(char);
	bool is_valid_path_char(char);
	bool is_valid_query_char(char);

	int parse_line();
	
	int parse_request_line();
	int parse_method(char*);
	int parse_url(char*);
	int parse_http_version(char*);
	int handle_path();

	int parse_header();
	int parse_body();

	int send_response();
	void send_buffer_append(string);

	void build_response_status_line();
	void build_response_date();
	void build_response_err();
	void build_response_ok();
};

bool cmp(EventData*, EventData*);

#endif
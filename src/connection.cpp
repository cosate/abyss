#include<unistd.h>
#include<time.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<sys/types.h>
#include<errno.h>
#include<cstring>
#include<ctype.h>

#include<string>
#include<vector>
#include<algorithm>

#include"config.h"
#include"connection.h"
#include"net.h"
#include"util.h"
#include"message.h"

#define PARSE_ERR (-1)
#define PARSE_OK (0)
#define PARSE_AGAIN (1)

extern vector<ConnectionData*> connections;
extern int epfd;
extern Config server_config;

bool cmp(EventData* c1, EventData* c2)
{
	return ((ConnectionData*)c1)->active_time > ((ConnectionData*)c2)->active_time;
}

int EventData::in_handler()
{
	return ABYSS_OK;
}

int EventData::out_handler()
{
	return ABYSS_OK;
}

EventData::~EventData()
{}

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
		ConnectionData* c = new ConnectionData(connfd);
		
		epoll_event connev;
		connev.events = EPOLLIN;
		connev.data.ptr = c;
		
		if(epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &connev) == ABYSS_ERR)
		{
			close(connfd);
			delete c;
			continue;
		}
		c->active_time = time(NULL);
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

void ConnectionData::construct()
{
	request = Request();
	response = Response();
	memset(send_buffer, 0, BUFFER_SIZE);
	memset(recv_buffer, 0, BUFFER_SIZE);
	recv_buffer_length = 0;
	send_buffer_length = 0;
	parse_status.section_begin = recv_buffer;
	parse_status.current = recv_buffer;
	parse_status.stage = Parse_Stage::PARSE_REQUEST_LINE;
	send_status.send_begin = send_buffer;
}

int ConnectionData::enable_in()
{
	if(!(this->events & EPOLLIN))
	{
		this->events |= EPOLLIN;

		epoll_event ev;
		ev.events = this->events;
		ev.data.ptr = this;
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
		ev.data.ptr = this;
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
		ev.data.ptr = this;
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
		ev.data.ptr = this;
		if(epoll_ctl(epfd, EPOLL_CTL_MOD, this->fd, &ev) == -1)
		{
			ABYSS_ERR_MSG(strerror(errno));
			return ABYSS_ERR;
		}
	}
	return ABYSS_OK;
}

#define BUFFER_ERR (-1)
#define BUFFER_OK (0)
#define BUFFER_AGAIN (1)
#define BUFFER_CLOSE (2)

int ConnectionData::recv_request()
{
	while(1)
	{
		ssize_t bytes = recv(this->fd, this->recv_buffer + this->recv_buffer_length, BUFFER_SIZE - this->recv_buffer_length, 0);
		if(bytes == 0)
			return BUFFER_CLOSE;
		else if(bytes == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				return BUFFER_AGAIN;
			else
				return BUFFER_ERR;
		}
		else
		{
			this->recv_buffer_length += bytes;
			if(BUFFER_SIZE == this->recv_buffer_length)
				return BUFFER_AGAIN;
		}
	}
}

int ConnectionData::send_response()
{
	while(this->send_status.send_begin - this->send_buffer < this->send_buffer_length)
	{
		ssize_t len = send(this->fd, this->send_status.send_begin, this->send_buffer_length - (this->send_status.send_begin - this->send_buffer), 0);
		if(len == -1)
			return (errno == EAGAIN || errno == EWOULDBLOCK) ? BUFFER_AGAIN : BUFFER_ERR;
		this->send_status.send_begin += len;
	}

	this->send_buffer_length = 0;
	this->send_status.send_begin = this->send_buffer;
	return BUFFER_OK;
}

int ConnectionData::in_handler()
{
	if(this->recv_request() != BUFFER_AGAIN)
	{
		return ABYSS_ERR;
	}
	
	switch(this->parse_request())
	{
		case PARSE_AGAIN:
			break;

		case PARSE_ERR:
		case PARSE_OK:
		{
			this->disable_in();
			this->enable_out();
			break;
		}
		default:
			{
				ABYSS_ERR_MSG("unknown return value from parse_request");
				exit(ABYSS_ERR);
			}
	}
	return ABYSS_OK;
}

int ConnectionData::out_handler()
{
	if(this->response.resource_fd != -1)
		set_tcp_cork(this->fd);

	switch(send_response())
	{
		case BUFFER_AGAIN:
			return ABYSS_OK;
		case BUFFER_ERR:
			return ABYSS_ERR;
		case BUFFER_OK:
		{
			if(this->response.resource_fd != -1)
			{
				struct stat st;
				fstat(this->response.resource_fd, &st);
				while(1)
				{
					ssize_t len = sendfile(this->fd, this->response.resource_fd, NULL, st.st_size);
					if(len == -1)
						return (errno == EAGAIN || errno == EWOULDBLOCK) ? ABYSS_OK : ABYSS_ERR;
					else if(len == 0)
					{
						close(this->response.resource_fd);
						reset_tcp_cork(this->fd);
						break;
					}
				}
			}

			if(this->response.status_code >= 300)
				return ABYSS_ERR;

			this->disable_out();
			this->enable_in();
			this->recv_buffer_length = 0;
			return ABYSS_OK;
		}
		default:
		{
			ABYSS_ERR_MSG("invalid send status");
			exit(ABYSS_ERR);
		}
	}

}

int ConnectionData::parse_line()
{
	while(this->parse_status.current < this->recv_buffer + recv_buffer_length)
	{
		if(*(this->parse_status.current) == '\r')
		{
			if(this->parse_status.current + 1 < this->recv_buffer + this->recv_buffer_length)
			{
				if(*(this->parse_status.current + 1) == '\n')
				{
					this->parse_status.current += 2;
					return PARSE_OK;
				}
				else
				{
					this->response.status_code = 400;
					return PARSE_ERR;
				}
			}
			else
				return PARSE_AGAIN;
		}
		else if(*(this->parse_status.current) == '\n')
		{
			if(this->parse_status.stage == Parse_Stage::PARSE_REQUEST_LINE)
			{
				this->response.status_code = 400;
				return PARSE_ERR;
			}
			else if(this->parse_status.stage == Parse_Stage::PARSE_HEADER)
			{
				if(this->parse_status.current + 1 < this->recv_buffer + this->recv_buffer_length)
				{
					if(*(this->parse_status.current + 1) == '\t' || *(this->parse_status.current + 1) == ' ')
					{
						this->parse_status.current += 1;
						continue;
					}
					else
					{
						this->response.status_code = 400;
						return PARSE_ERR;
					}
				}
				else
					return PARSE_AGAIN;
			}
		}
		this->parse_status.current++;
	}
	return PARSE_AGAIN;
}

int ConnectionData::parse_request_line()
{
	if(this->parse_status.stage != Parse_Stage::PARSE_REQUEST_LINE)
		return PARSE_ERR;
	this->parse_status.stage = Parse_Stage::PARSE_METHOD;
	this->response.status_code = 400;
	this->pass_whitespace();
	for(char* p = this->parse_status.section_begin; p < this->parse_status.current; p++)
	{
		if(*p == ' ' || *p == '\r')
		{
			switch(this->parse_status.stage)
			{
				case Parse_Stage::PARSE_METHOD:
				{
					if(parse_method(p) == PARSE_OK)
					{
						p = this->parse_status.section_begin;
						break;
					}
					else
						return PARSE_ERR;
				}
				case Parse_Stage::PARSE_URL:
				{
					if(parse_url(p) == PARSE_OK)
					{
						p = this->parse_status.section_begin;
						break;
					}
					else
						return PARSE_ERR;
				}
				case Parse_Stage::PARSE_HTTP_VERSION:
				{
					if(parse_http_version(p) == PARSE_OK)
					{
						p = this->parse_status.section_begin;
						break;
					}
					else
						return PARSE_ERR;
				}
				default:
				{
					ABYSS_ERR_MSG("invalid parse stage in parse_request_line");
					exit(ABYSS_ERR);
				}
			}
		}
	}

	if(this->parse_status.stage != Parse_Stage::PARSE_HEADER)
	{
		this->response.status_code = 400;
		return PARSE_ERR;
	}
	return PARSE_OK;
}

void ConnectionData::pass_whitespace()
{
	while(this->parse_status.section_begin < this->parse_status.current)
	{
		if(*(this->parse_status.section_begin) == ' ' || *(this->parse_status.section_begin) == '\t')
			this->parse_status.section_begin++;
		else
			break;
	}
}

int ConnectionData::parse_method(char* end)
{
	if(this->parse_status.stage != Parse_Stage::PARSE_METHOD)
		return PARSE_ERR;
	this->response.status_code = 400;
	switch(end - this->parse_status.section_begin)
	{
		case 3:
		{
			if(strncmp(this->parse_status.section_begin, "GET", 3) == 0)
			{
				this->request.method = Method::GET;
				break;
			}
			else if(strncmp(this->parse_status.section_begin, "PUT", 3) == 0)
			{
				this->request.method = Method::PUT;
				break;
			}
			return PARSE_ERR;
		}

		case 4:
		{
			if(strncmp(this->parse_status.section_begin, "POST", 4) == 0)
			{
				this->request.method = Method::POST;
				break;
			}
			else if(strncmp(this->parse_status.section_begin, "HEAD", 4) == 0)
			{
				this->request.method = Method::HEAD;
				break;
			}
			return PARSE_ERR;
		}

		case 5:
		{
			if(strncmp(this->parse_status.section_begin, "TRACE", 5) == 0)
			{
				this->request.method = Method::TRACE;
				break;
			}
			return PARSE_ERR;
		}

		case 6:
		{
			if(strncmp(this->parse_status.section_begin, "DELETE", 6) == 0)
			{
				this->request.method = Method::DELETE;
				break;
			}
			return PARSE_ERR;
		}

		case 7:
		{
			if(strncmp(this->parse_status.section_begin, "OPTIONS", 7) == 0)
			{
				this->request.method = Method::OPTIONS;
				break;
			}
			else if(strncmp(this->parse_status.section_begin, "CONNECT", 7) == 0)
			{
				this->request.method = Method::CONNECT;
				break;
			}
			return PARSE_ERR;
		}

		default:
			return PARSE_ERR;
	}

	this->response.status_code = 200;
	this->parse_status.section_begin = end;
	this->pass_whitespace();
	this->parse_status.stage = Parse_Stage::PARSE_URL;
	return PARSE_OK;
}

bool ConnectionData::is_valid_scheme_char(char ch)
{
	switch(ch)
	{
		case '-': /* gsm-sms view-source */
		case '+': /* whois++ */
		case '.': /* z39.50r z39.50s */
			return true;
		default:
			if(isalnum(ch))
				return true;
	}
	return false;
}

bool ConnectionData::is_valid_host_char(char ch)
{
	switch(ch)
	{
		case '-':
		case '.':
			return true;
		default:
			if(isalnum(ch))
				return true;
	}
	return false;
}

/*TODO: %*/
bool ConnectionData::is_valid_path_char(char ch)
{
	switch(ch)
	{
		case '{':
		case '}':
		case '|':
		case '\\':
		case '^':
		case '~':
		case '[':
		case ']':
		case '\'':
		case '<':
		case '>':
		case '"':
		case ' ':
			return false;
		default:
			if((ch >= 0x00 && ch <= 0x1F) || ch >= 0x7F)
				return false;
	}
	return true;
}

bool ConnectionData::is_valid_query_char(char ch)
{
	switch(ch)
	{
		case '=':
		case '&':
		case '%':
			return true;

		default:
			if(isalnum(ch))
				return true;
	}
	return false;
}

/*General: <scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<frag>*/
/*Implement: [<http>://<host>[:<port>]][/<path>[?<query>]]*/
int ConnectionData::parse_url(char* end)
{
	if(this->parse_status.stage != Parse_Stage::PARSE_URL)
		return PARSE_ERR;

	this->response.status_code = 400;
	for(char* p = this->parse_status.section_begin; p < end; p++)
	{
		switch(this->parse_status.stage)
		{
			case Parse_Stage::PARSE_URL:
			{
				switch(*p)
				{
					case 'h':
					case 'H':
					{
						this->parse_status.stage = Parse_Stage::PARSE_URL_SCHEME;
						p--;
						break;
					}
					case '/':
					{
						this->parse_status.stage = Parse_Stage::PARSE_URL_PATH;
						this->parse_status.section_begin = p + 1;
						break;
					}
					default:
						return PARSE_ERR;
				}
				break;
			}

			case Parse_Stage::PARSE_URL_SCHEME:
			{
				switch(*p)
				{
					case ':':
					{
						if(*(p+1) == '/' && *(p+2) == '/')
						{
							this->parse_status.stage = Parse_Stage::PARSE_URL_HOST;
							this->request.url.scheme.str = this->parse_status.section_begin;
							this->request.url.scheme.len = p - this->parse_status.section_begin;
							this->parse_status.section_begin = p + 3;
							p += 2;
							break;
						}
						else
							return PARSE_ERR;
					}
					default:
						if(!is_valid_scheme_char(*p))
							return PARSE_ERR;
						break;
				}
				break;
			}

			case Parse_Stage::PARSE_URL_HOST:
			{
				switch(*p)
				{
					case ':':
					{
						this->parse_status.stage = Parse_Stage::PARSE_URL_PORT;
						this->request.url.host.str = this->parse_status.section_begin;
						this->request.url.host.len = p - this->parse_status.section_begin;
						this->parse_status.section_begin = p + 1;
						break;
					}
					case '/':
					{
						this->parse_status.stage = Parse_Stage::PARSE_URL_PATH;
						this->request.url.host.str = this->parse_status.section_begin;
						this->request.url.host.len = p - this->parse_status.section_begin;
						this->parse_status.section_begin = p + 1;
						break;
					}
					default:
						if(!is_valid_host_char(*p))
							return PARSE_ERR;
						break;
				}
				break;
			}

			case Parse_Stage::PARSE_URL_PORT:
			{
				switch(*p)
				{
					case '/':
					{
						this->parse_status.stage = Parse_Stage::PARSE_URL_PATH;
						this->request.url.port.str = this->parse_status.section_begin;
						this->request.url.port.len = p - this->parse_status.section_begin;
						this->parse_status.section_begin = p + 1;
						break;
					}
					default:
						if(!isdigit(*p))
							return PARSE_ERR;
						break;
				}
				break;
			}

			case Parse_Stage::PARSE_URL_PATH:
			{
				switch(*p)
				{
					case '?':
					{
						this->parse_status.stage = Parse_Stage::PARSE_URL_QUERY;
						this->request.url.path.str = this->parse_status.section_begin;
						this->request.url.path.len = p - this->parse_status.section_begin;
						if(this->request.url.extension.str)
						{
							this->request.url.extension.len = p - this->request.url.extension.str;
						}
						this->parse_status.section_begin = p + 1;
						break;
					}
					case '.':
					{
						this->request.url.extension.str = p + 1;
						break;
					}
					default:
						if(!is_valid_path_char(*p))
							return PARSE_ERR;
						break;
				}
				break;
			}

			case Parse_Stage::PARSE_URL_QUERY:
			{
				switch(*p)
				{
					/* TODO */
					default:
						if(!is_valid_query_char(*p))
							return PARSE_ERR;
					break;
				}
				break;
			}

			default:
			{
				ABYSS_ERR_MSG("invalid stage in parse_url");
				exit(ABYSS_ERR);
			}
		}
	}

	switch(this->parse_status.stage)
	{
		case Parse_Stage::PARSE_URL_HOST:
		{
			if(this->parse_status.section_begin == end)
			{
				return PARSE_ERR;
			}
			else
			{
				this->request.url.host.str = this->parse_status.section_begin;
				this->request.url.host.len = end - this->parse_status.section_begin;
			}
			break;
		}
		case Parse_Stage::PARSE_URL_PORT:
		{
			if(this->parse_status.section_begin == end)
			{
				return PARSE_ERR;
			}
			else
			{
				this->request.url.port.str = this->parse_status.section_begin;
				this->request.url.port.len = end - this->parse_status.section_begin;
			}
			break;
		}
		case Parse_Stage::PARSE_URL_PATH:
		{
			this->request.url.path.str = this->parse_status.section_begin;
			this->request.url.path.len = end - this->parse_status.section_begin;
			if(this->request.url.extension.str)
				this->request.url.extension.len = end - this->request.url.extension.str;
			break;
		}
		case Parse_Stage::PARSE_URL_QUERY:
		{
			if(this->parse_status.section_begin == end)
			{
				return PARSE_ERR;
			}
			else
			{
				this->request.url.query.str = this->parse_status.section_begin;
				this->request.url.query.len = end - this->parse_status.section_begin;
			}
			break;
		}
		default:
			return PARSE_ERR;
	}
	this->response.status_code = 200;
	this->parse_status.section_begin = end;
	this->pass_whitespace();
	this->parse_status.stage = Parse_Stage::PARSE_HTTP_VERSION;
	return PARSE_OK;
}

int ConnectionData::parse_http_version(char* end)
{
	if(this->parse_status.stage != Parse_Stage::PARSE_HTTP_VERSION)
		return PARSE_ERR;

	this->response.status_code = 400;

	if(end - this->parse_status.section_begin <= 5 || strncasecmp(this->parse_status.section_begin, "http/", 5))
		return PARSE_ERR;

	char* p = this->parse_status.section_begin;
	for(p += 5; p < end && *p != '.'; p++)
	{
		cout<<*p<<endl;
		if(isdigit(*p))
			this->request.http_version.major_version = this->request.http_version.major_version * 10 + (*p) - '0';
		else
			return PARSE_ERR;
	}

	if(p == end || p + 1 == end)
		return PARSE_ERR;

	for(p += 1; p < end; p++)
	{
		cout<<*p<<endl;
		if(isdigit(*p))
			this->request.http_version.minor_version = this->request.http_version.minor_version * 10 + (*p) - '0';
		else
			return PARSE_ERR;
	}

	if(this->request.http_version.major_version != 1 || (this->request.http_version.minor_version != 0 && this->request.http_version.minor_version != 1))
	{
		this->response.status_code = 505;
		return PARSE_ERR;
	}

	this->response.status_code = 200;
	this->parse_status.section_begin = this->parse_status.current;
	this->parse_status.stage = Parse_Stage::PARSE_HEADER;
	return PARSE_OK;
}

int ConnectionData::parse_header()
{
	if(this->parse_status.stage != Parse_Stage::PARSE_HEADER)
		return PARSE_ERR;

	if(this->parse_status.current - this->parse_status.section_begin == 2)
	{
		if(this->request.http_version.major_version == 1 && this->request.http_version.minor_version == 1 && this->request.header.host.len == 0)
		{
			this->response.status_code = 400;
			return PARSE_ERR;
		}
		this->parse_status.stage = Parse_Stage::PARSE_BODY;
		this->parse_status.section_begin = this->parse_status.current;
		return PARSE_OK;
	}

	this->response.status_code = 400;
	this->parse_status.stage = Parse_Stage::PARSE_HEADER_NAME;
	Str name = Str();
	Str value = Str();
	for(char* p = this->parse_status.section_begin; p < this->parse_status.current; p++)
	{
		switch(this->parse_status.stage)
		{
			case Parse_Stage::PARSE_HEADER_NAME:
			{
				switch(*p)
				{
					case ':':
					{
						name.str = this->parse_status.section_begin;
						name.len = p - this->parse_status.section_begin;
						this->parse_status.stage = Parse_Stage::PARSE_HEADER_VALUE;
						this->parse_status.section_begin = p + 1;
						this->pass_whitespace();
						break;
					}
					case '-':
					{
						*p = '_';
						break;
					}
					default:
					{
						if(!isalnum(*p))
							return PARSE_ERR;
						if('A' <= *p && 'Z' >= *p)
							*p = *p - 'A' + 'a';
						break;
					}
				}
				break;
			}
			case Parse_Stage::PARSE_HEADER_VALUE:
			{
				switch(*p)
				{
					case '\r':
					{
						value.str = this->parse_status.section_begin;
						value.len = p - this->parse_status.section_begin;
						break;
					}
					default:
						break;
				}
				break;
			}
			default:
			{
				ABYSS_ERR_MSG("invalid stage in parse_header");
				exit(ABYSS_ERR);
			}
		}
	}
	this->response.status_code = 200;
	Str* header = (Str*)(((char*)&(this->request.header)) + this->request.header.field2position[string(name.str, name.str + name.len)]);
	header->str = value.str;
	header->len = value.len;

	this->parse_status.stage = Parse_Stage::PARSE_HEADER;
	this->parse_status.section_begin = this->parse_status.current;
	return PARSE_OK;
}

int ConnectionData::parse_body()
{
	if(this->parse_status.stage != Parse_Stage::PARSE_BODY)
		return PARSE_ERR;

	switch(this->request.method)
	{
		case Method::PUT:
		case Method::POST:
		{
			if(this->request.header.content_length.str == NULL && this->request.header.content_length.len == 0)
			{
				this->response.status_code = 400;
				return PARSE_ERR;
			}
			size_t contlength = 0;
			for(int i = 0; i < this->request.header.content_length.len; i++)
			{
				if(!isdigit(*(this->request.header.content_length.str + i)))
				{
					this->response.status_code = 400;
					return PARSE_ERR;
				}
				contlength = contlength * 10 + *(this->request.header.content_length.str + i) - '0';
			}
			if(this->recv_buffer_length < contlength)
				return PARSE_AGAIN;

			this->request.body.str = this->parse_status.section_begin;
			this->request.body.len = contlength;
			this->parse_status.stage = Parse_Stage::PARSE_DONE;
			this->parse_status.section_begin += contlength;
			return PARSE_OK;
		}
		default:
		{
			if(this->request.header.content_length.str != NULL && this->request.header.content_length.len != 0)
			{
				this->response.status_code = 400;
				return PARSE_ERR;
			}
			this->parse_status.stage = Parse_Stage::PARSE_DONE;
			return PARSE_OK;
		}
	}
}

void ConnectionData::build_response_status_line()
{
	this->response.http_version.major_version = this->request.http_version.major_version;
	this->response.http_version.minor_version = this->request.http_version.minor_version;

	int n = sprintf(this->send_buffer, "HTTP/%d.%d ", this->response.http_version.major_version, this->response.http_version.minor_version);
	this->send_buffer_length += n;

	this->response.code_description = this->response.code2description[this->response.status_code];
	send_buffer_append(this->response.code_description);

	send_buffer_append("\r\n");
}

void ConnectionData::build_response_date()
{
	time_t t = time(NULL);
	tm* time = localtime(&t);
	int n = strftime(this->send_buffer + this->send_buffer_length, BUFFER_SIZE - this->send_buffer_length,
				"Date: %a, %d %b, %y, %H:%M:%S GMT\r\n", time);
	this->send_buffer_length += n;
}

void ConnectionData::send_buffer_append(string s)
{
	memcpy(this->send_buffer + this->send_buffer_length, s.c_str(), s.length());
	this->send_buffer_length += s.length();
}

void ConnectionData::build_response_ok()
{
	//For now GET only
	if(this->request.method != Method::GET)
	{
		this->response.status_code = 501;
		this->build_response_err();
		return;
	}

	switch(this->response.status_code)
	{
		case 100:
		case 101:
		case 201:
		case 202:
		case 203:
		case 204:
		case 205:
		case 206:
		{
			/* TODO */
			break;
		}
		case 200:
			break;

		default:
		{
			ABYSS_ERR_MSG("invalid status code in build_response_ok");
			exit(ABYSS_ERR);
		}
	}

	build_response_status_line();

	send_buffer_append("Server: Abyss\r\n");

	build_response_date();

	send_buffer_append("Content-Type: ");
	if(this->request.url.extension.str != NULL)
		send_buffer_append(this->response.mime[string(this->request.url.extension.str, this->request.url.extension.str + this->request.url.extension.len)]);
	else
		send_buffer_append("text/html");

	send_buffer_append("\r\n");
	send_buffer_append("Content-Length: ");

	struct stat status;
	fstat(this->response.resource_fd, &status);
	int contlength = status.st_size;
	int n = sprintf(this->send_buffer + this->send_buffer_length, "%d\r\n", contlength);
	this->send_buffer_length += n;

	send_buffer_append("\r\n");
}

void ConnectionData::build_response_err()
{
	if(this->response.status_code < 300)
	{
		ABYSS_ERR_MSG("wrong status code");
		exit(ABYSS_ERR);
	}

	build_response_status_line();
	send_buffer_append("Server: Abyss\r\n");
	build_response_date();

	send_buffer_append("Connection: close\r\n");
	send_buffer_append("Content-Type: text/html\r\n");
	send_buffer_append("Content-Length: ");

	if(this->response.resource_fd != -1)
		close(this->response.resource_fd);

	char err_file[10];
	snprintf(err_file, 10, "%d.html",this->response.status_code);
	this->response.resource_fd = openat(server_config.err_root, err_file, O_RDONLY);
	if(this->response.resource_fd == -1)
	{
		ABYSS_ERR_MSG("error file not exist");
		exit(ABYSS_ERR);
	}

	struct stat status;
	fstat(this->response.resource_fd, &status);

	int contlength = status.st_size;
	int n = sprintf(this->send_buffer + this->send_buffer_length, "%d\r\n", contlength);
	this->send_buffer_length += n;

	send_buffer_append("\r\n");
}

int ConnectionData::parse_request()
{
	while(this->parse_status.stage != Parse_Stage::PARSE_BODY)
	{
		switch(parse_line())
		{
			case PARSE_OK:
			{
				switch(this->parse_status.stage)
				{
					case Parse_Stage::PARSE_REQUEST_LINE:
					{
						if(parse_request_line() != PARSE_OK)
						{
							build_response_err();
							return PARSE_ERR;
						}
						break;
					}
					case Parse_Stage::PARSE_HEADER:
					{
						if(parse_header() != PARSE_OK)
						{
							build_response_err();
							return PARSE_ERR;
						}
						break;
					}
					default:
					{
						ABYSS_ERR_MSG("invalid parse stage of request");
						exit(ABYSS_ERR);
					}
				}
				break;
			}
			case PARSE_ERR:
			{
				build_response_err();
				return PARSE_ERR;
			}
			case PARSE_AGAIN:
			{
				return PARSE_AGAIN;
			}
			default:
			{
				ABYSS_ERR_MSG("invalid parse stage of request");
				exit(ABYSS_ERR);
			}
		}
	}

	switch(parse_body())
	{
		case PARSE_ERR:
		{
			build_response_err();
			return PARSE_ERR;
		}
		case PARSE_AGAIN:
		{
			return PARSE_AGAIN;
		}
		case PARSE_OK:
		{
			if(this->parse_status.stage != Parse_Stage::PARSE_DONE)
				return PARSE_ERR;

			if(handle_path() == PARSE_OK)
			{
				build_response_ok();
				return PARSE_OK;
			}
			else
			{
				build_response_err();
				return PARSE_ERR;
			}
		}
		default:
		{
			ABYSS_ERR_MSG("invalid parse stage of request");
			exit(ABYSS_ERR);
		}
	}
}

int ConnectionData::handle_path()
{
	string path;
	if(this->request.url.path.str == NULL && this->request.url.path.len == 0)
		path = "./";
	else
		path = string(this->request.url.path.str, this->request.url.path.str + this->request.url.path.len);

	int fd = openat(server_config.src_root, path.c_str(), O_RDONLY);
	if(fd == -1)
	{
		this->response.status_code = 404;
		return PARSE_ERR;
	}

	struct stat status;
	fstat(fd, &status);
	if(S_ISDIR(status.st_mode))
	{
		int dir_fd = fd;
		fd = openat(dir_fd, "index.html", O_RDONLY);
		if(fd == -1)
		{
			this->response.status_code = 404;
			return PARSE_ERR;
		}
	}

	this->response.resource_fd = fd;
	return PARSE_OK;
}
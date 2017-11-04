class EventData
{
public:
	int fd;
	virtual int in_handler() = 0;
	EventData() : fd(-1) {}
	EventData(int f) :fd(f) {}
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
	void construct();
};

void ConnectionData::construct()
{
	request = Request();
	response = Response();
	memset(send_buffer, 0, BUFFERSIZE);
	memset(recv_buffer, 0, BUFFERSIZE);
	buffer_length = 0;
}

int ConnectionData::in_handler()
{

}

int ConnectionData::out_handler()
{

}

class ListenData : public EventData
{
public:
	ListenData() : EventData() {}
	ListenData(int f) : EventData(f) {}

	int in_handler();
}

int EventData::in_handler()
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

#define PARSE_ERR (-1)
#define PARSE_OK (0)
#define PARSE_AGAIN (1)

int parse_request()
{

}

int ConnectionData::parse_line()
{
	while(parse_status.current < recv_buffer + buffer_length)
	{
		if(*(parse_status.current) == '\r')
		{
			if(parse_status.current + 1 < recv_buffer + buffer_length)
			{
				if(*(parse_status.current + 1) == '\n')
				{
					parse_status.current += 2;
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
		else if(*(parse_status.current) == '\n')
		{
			if(parse_status.stage == Parse_Stage::PARSE_REQUEST_LINE)
			{
				this->response.status_code = 400;
				return PARSE_ERR;
			}
			else if(parse_status.stage == PARSE_HEADER)
			{
				if(parse_status.current + 1 < recv_buffer + buffer_length)
				{
					if(*(parse_status.current + 1) == '\t' || *(parse_status.current + 1) == ' ')
					{
						parse_status.current += 2;
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
		parse_status.current++;
	}
	return PARSE_AGAIN;
}

int ConnectionData::parse_request_line()
{
	if(this->parse_status.stage != Parse_Stage::PARSE_REQUEST_LINE)
		return PARSE_ERR;
	this->parse_status.stage = PARSE_METHOD;
	this->response.status_code = 400;
	this->pass_whitespace();
	for(char* p == this->parse_status.section_begin; p < this->parse_status.current; p++)
	{
		if(*p == ' ' || *p = '\r')
		{
			switch(this->parse_status)
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
					if(parse_method(p) == PARSE_OK)
					{
						p = this->parse_status.section_begin;
						break;
					}
					else
						return PARSE_ERR;
				}
				default:
					return PARSE_ERR;
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
		if(this->parse_status.section_begin == ' ' || this->parse_status.section_begin == '\t')
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
bool is_valid_path_char(char ch)
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
			this->request.url.port.str = this->parse_status.section_begin;
			this->request.url.port.len = end - this->parse_status.section_begin;
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
	for(; p < end && *p != '.'; p++)
	{
		if(isdigit(*p))
			this->request.http_version.major = this->request.http_version.major * 10 + (*p) - '0';
		else
			return PARSE_ERR;
	}

	if(p == end || p + 1 == end)
		return PARSE_ERR;

	for(p += 1; p < end; p++)
	{
		if(isdigit(*p))
			this->request.http_version.minor = this->request.http_version.minor * 10 + (*p) - '0';
		else
			return PARSE_ERR;
	}

	if(this->request.http_version.major != 1 || (this->request.http_version.minor != 0 && this->request.http_version.minor != 1))
	{
		this->response.status_code = 505;
		return PARSE_ERR;
	}

	this->response.status_code = 200;
	this->parse_status.section_begin = this->parse_status.current;
	this->parse_status.stage = Parse_Stage::PARSE_HEADER;
	return PARSE_OK;
}
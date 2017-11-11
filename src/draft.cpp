#define PARSE_ERR (-1)
#define PARSE_OK (0)
#define PARSE_AGAIN (1)

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
			else if(this->parse_status.stage == PARSE_HEADER)
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

int parse_header()
{
	if(this->parse_status.stage != Parse_Stage::PARSE_HEADER)
		return PARSE_ERR;

	if(this->parse_status.current - this->parse_status == 2)
	{
		if(this->request.http_version.major == 1 && this->request.http_version.minor == 1 && this->request.header.host.len == 0)
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
	Str name();
	Str value();
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
						name.str = this->section_begin;
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
							return Parse_Stage::PARSE_ERR;
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
	Str* header = (Str*)(((char*)&(this->request.header)) + field2position[string(name.str, name.str + name.len)]);
	header->str = value.str;
	header->len = value.len;

	this->parse_status.stage = Parse_Stage::PARSE_HEADER;
	this->parse_status.section_begin = this->parse_status.current;
	return PARSE_OK;
}

int parse_body()
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
			this->parse_status = PARSE_DONE;
			return PARSE_OK;
		}
	}
}

void ConnectionData::build_response_status_line()
{
	this->response.http_version.major = this->request.http_version.major;
	this->response.http_version.minor = this->request.http_version.minor;

	int n = sprintf(this->send_buffer, "HTTP/%d.%d", this->response.http_version.major, this->response.http_version.minor);
	this->send_buffer_length += n;

	this->response.code_description = this->code2description[this->response.status_code];
	send_buffer_append(this->response.code_description)

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
	memcpy(this->send_buffer + this->send_buffer_length, s);
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
	if(this->request.url.extension != NULL)
		send_buffer_append(mime[string(this->request.url.extension.str, this->request.url.extension.str + this->request.url.extension.len)]);
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

	if(this->response.resource_fd != -1)
		close(this->response.resource_fd);

	

}

int ConnectionData::parse_request()
{
	while(this->parse_status.stage != PARSE_BODY)
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

	int fd = openat(server_config.src_root, path, O_RDONLY);
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
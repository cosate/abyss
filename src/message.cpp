#include<map>
#include"message.h"

using namespace std;

static map<int, string> code2description;

static init_code_description()
{
	code2description[100] = "100 Continue";
	code2description[101] = "101 Switching Protocols";
	code2description[200] = "200 OK";
	code2description[201] = "201 Created";
	code2description[202] = "202 Accepted";
	code2description[203] = "203 Non-Authoritative Information";
	code2description[204] = "204 No Content";
	code2description[205] = "205 Reset Content";
	code2description[206] = "206 Partial Content";
	code2description[300] = "300 Multiple Choices";
	code2description[301] = "301 Moved Permanently";
	code2description[302] = "302 Found";
	code2description[303] = "303 See Other";
	code2description[304] = "304 Not Modified";
	code2description[305] = "305 Use Proxy";
	code2description[307] = "307 Temporary Redirect";
	code2description[400] = "400 Bad Request";
	code2description[401] = "401 Unauthorized";
	code2description[402] = "402 Payment Required";
	code2description[403] = "403 Forbidden";
	code2description[404] = "404 Not Found";
	code2description[405] = "405 Method Not Allowed";
	code2description[406] = "406 Not Acceptable";
	code2description[407] = "407 Proxy Authentication Required";
	code2description[408] = "408 Request Timeout";
	code2description[409] = "409 Conflict";
	code2description[410] = "410 Gone";
	code2description[411] = "411 Length Required";
	code2description[412] = "412 Precondition Failed";
	code2description[413] = "413 Request Entity Too Large";
	code2description[414] = "414 Request URI Too Long";
	code2description[415] = "415 Unsupported Media Type";
	code2description[416] = "416 Requested Range Not Satisfiable";
	code2description[417] = "417 Expectation Failed";
	code2description[500] = "500 Internal Server Error";
	code2description[501] = "501 Not Implemented";
	code2description[502] = "502 Bad Gateway";
	code2description[503] = "503 Service Unavailable";
	code2description[504] = "504 Gateway Timeout";
	code2description[505] = "505 HTTP Version Not Supported";
}

int parse_request(Connection& connection)
{
	char* line_begin = connection.recv_buffer;
	char* line_end = connection.recv_buffer;

	parse_line(line_begin, line_end);
	parse_request_line(line_begin, line_end);

	while(parse_line(line_begin, line_end) && !(line_begin + 2 == line_end))
	{
		parse_header()
	}

	parse_line(line_begin, line_end);
	parse_body(line_begin, line_end);
}

int parse_line(char* & begin, char* & end)
{
	char* p = begin;
	while(p != '/r')
	{
		p++;
	}
}
int parse_request_line(char* begin, char* end);
int parse_header(char* begin, char* end);
int parse_body(char* begin, char* end);

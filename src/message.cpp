#include<map>
#include"message.h"

using namespace std;

map<int, string> Response:: code2description = map<int, string>();

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

map<string, size_t> RequestHeader::field2position = map<string, size_t>();

static RequestHeader::init_field_position()
{
	/* general headers */
    field2position["connection"] = offsetof(RequestHeader, connection)
    field2position["date"] = offsetof(RequestHeader, date)
    field2position["mime_version"] = offsetof(RequestHeader, mime_version)
    field2position["trailer"] = offsetof(RequestHeader, trailer)
    field2position["transfer_encoding"] = offsetof(RequestHeader, transfer_encoding)
    field2position["update"] = offsetof(RequestHeader, update)
    field2position["via"] = offsetof(RequestHeader, via)
    field2position["cache_control"] = offsetof(RequestHeader, cache_control)
    field2position["pragma"] = offsetof(RequestHeader, pragma)
    /* request headers */
    field2position["client_ip"] = offsetof(RequestHeader, client_ip)
    field2position["from"] = offsetof(RequestHeader, from)
    field2position["host"] = offsetof(RequestHeader, host)
    field2position["referer"] = offsetof(RequestHeader, referer)
    field2position["ua_color"] = offsetof(RequestHeader, ua_color)
    field2position["ua_cpu"] = offsetof(RequestHeader, ua_cpu)
    field2position["ua_disp"] = offsetof(RequestHeader, ua_disp)
    field2position["ua_os"] = offsetof(RequestHeader, ua_os)
    field2position["ua_pixels"] = offsetof(RequestHeader, ua_pixels)
    field2position["user_agent"] = offsetof(RequestHeader, user_agent)
    field2position["accept"] = offsetof(RequestHeader, accept)
    field2position["accept_charset"] = offsetof(RequestHeader, accept_charset)
    field2position["accept_encoding"] = offsetof(RequestHeader, accept_encoding)
    field2position["accept_language"] = offsetof(RequestHeader, accept_language)
    field2position["te"] = offsetof(RequestHeader, te)
    field2position["expect"] = offsetof(RequestHeader, expect)
    field2position["if_match"] = offsetof(RequestHeader, if_match)
    field2position["if_modified_since"] = offsetof(RequestHeader, if_modified_since)
    field2position["if_none_match"] = offsetof(RequestHeader, if_none_match)
    field2position["if_range"] = offsetof(RequestHeader, if_range)
    field2position["if_unmodified_since"] = offsetof(RequestHeader, if_unmodified_since)
    field2position["range"] = offsetof(RequestHeader, range)
    field2position["authorization"] = offsetof(RequestHeader, authorization)
    field2position["cookie"] = offsetof(RequestHeader, cookie)
    field2position["cookie2"] = offsetof(RequestHeader, cookie2)
    field2position["max_forward"] = offsetof(RequestHeader, max_forward)
    field2position["proxy_authorization"] = offsetof(RequestHeader, proxy_authorization)
    field2position["proxy_connection"] = offsetof(RequestHeader, proxy_connection)
    /* entity headers */
    field2position["allow"] = offsetof(RequestHeader, allow)
    field2position["location"] = offsetof(RequestHeader, location)
    field2position["content_base"] = offsetof(RequestHeader, content_base)
    field2position["content_encoding"] = offsetof(RequestHeader, content_encoding)
    field2position["content_language"] = offsetof(RequestHeader, content_language)
    field2position["content_length"] = offsetof(RequestHeader, content_length)
    field2position["content_location"] = offsetof(RequestHeader, content_location)
    field2position["content_md5"] = offsetof(RequestHeader, content_md5)
    field2position["content_range"] = offsetof(RequestHeader, content_range)
    field2position["content_type"] = offsetof(RequestHeader, content_type)
    field2position["etag"] = offsetof(RequestHeader, etag)
    field2position["expires"] = offsetof(RequestHeader, expires)
    field2position["last_modified"] = offsetof(RequestHeader, last_modified)
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

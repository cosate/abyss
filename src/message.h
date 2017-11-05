#ifndef ABYSS_REQUEST_H
#define ABYSS_REQUEST_H

#include<string>
#include<map>

#include"connection.h"
#include"str.h"
using namespace std;

enum class Method {GET = 0, PUT, HEAD, POST, TRACE, DELETE, CONNECT, OPTIONS};

class Url
{
public:
	Str scheme;
	Str host;
	Str port;
	Str path;
	Str extension;
	Str query;
	Url()
	{
		scheme = Str();
		host = Str();
		port = Str();
		path = Str();
		extension = Str();
		query = Str();
	}
};

class Version
{
public:
	int major;
	int minor;
	Version() : major(0), minor(0) {}
};

class RequestHeader
{
public:
	/* general headers */
    Str connection;
    Str date;
    Str mime_version;
    Str trailer;
    Str transfer_encoding;
    Str update;
    Str via;
    Str cache_control;
    Str pragma;
    /* request headers */
    Str client_ip;
    Str from;
    Str host;
    Str referer;
    Str ua_color;
    Str ua_cpu;
    Str ua_disp;
    Str ua_os;
    Str ua_pixels;
    Str user_agent;
    Str accept;
    Str accept_charset;
    Str accept_encoding;
    Str accept_language;
    Str te;
    Str expect;
    Str if_match;
    Str if_modified_since;
    Str if_none_match;
    Str if_range;
    Str if_unmodified_since;
    Str range;
    Str authorization;
    Str cookie;
    Str cookie2;
    Str max_forward;
    Str proxy_authorization;
    Str proxy_connection;
    /* entity headers */
    Str allow;
    Str location;
    Str content_base;
    Str content_encoding;
    Str content_language;
    Str content_length;
    Str content_location;
    Str content_md5;
    Str content_range;
    Str content_type;
    Str etag;
    Str expires;
    Str last_modified;
    RequestHeader()
    {
    	connection = Str()
	    date = Str()
	    mime_version = Str()
	    trailer = Str()
	    transfer_encoding = Str()
	    update = Str()
	    via = Str()
	    cache_control = Str()
	    pragma = Str()
	    /* request headers */
	    client_ip = Str()
	    from = Str()
	    host = Str()
	    referer = Str()
	    ua_color = Str()
	    ua_cpu = Str()
	    ua_disp = Str()
	    ua_os = Str()
	    ua_pixels = Str()
	    user_agent = Str()
	    accept = Str()
	    accept_charset = Str()
	    accept_encoding = Str()
	    accept_language = Str()
	    te = Str()
	    expect = Str()
	    if_match = Str()
	    if_modified_since = Str()
	    if_none_match = Str()
	    if_range = Str()
	    if_unmodified_since = Str()
	    range = Str()
	    authorization = Str()
	    cookie = Str()
	    cookie2 = Str()
	    max_forward = Str()
	    proxy_authorization = Str()
	    proxy_connection = Str()
	    /* entity headers */
	    allow = Str()
	    location = Str()
	    content_base = Str()
	    content_encoding = Str()
	    content_language = Str()
	    content_length = Str()
	    content_location = Str()
	    content_md5 = Str()
	    content_range = Str()
	    content_type = Str()
	    etag = Str()
	    expires = Str()
	    last_modified = Str()
    }

    static map<string, size_t> field2position;
    static init_field_position();
};

class Request
{
public:
	Method method;
	Url url;
	Version http_version;
	RequestHeader header;
	Str body;
	Request()
	{
		method = Method::GET;
		url = Url();
		version = Version();
		body = Str();
		header = RequestHeader();
	}
};

class Response
{
public:
	Version version;
	int status_code;
	string code_description;
	map<Str, Str> header;
	Str body;
	Response()
	{
		version = Version();
		status_code = 200;
		code_description = "200 OK";
		body = Str();
		header = map<Str, Str>();
	}

	static map<int, string> code2description;
	static init_code_description();
};

#endif
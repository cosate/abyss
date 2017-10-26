#ifndef ABYSS_REQUEST_H
#define ABYSS_REQUEST_H

#include<string>
#include<map>

#include"connection.h"
using namespace std;

enum class Method {GET = 0, PUT, HEAD, POST, TRACE, DELETE, CONNECT, OPTIONS};

class Url
{
public:
	string scheme;
	string host;
	string port;
	string path;
	string extension;
	string query;
	Url()
	{
		scheme = "";
		host = "";
		port = "";
		path = "";
		extension = "";
		query = "";
	}
};

class Version
{
public:
	int major;
	int minor;
	Version() : major(1), minor(0) {}
};

enum class Status {UNPARSED = 0, PARSE_REQUEST_LINE, PARSE_HEADER, PARSE_BODY};

class Request
{
public:
	Method method;
	Url url;
	Version version;
	map<string, string> header;
	string body;
	Status status;
	Request()
	{
		method = Method::GET;
		url = Url();
		version = Version();
		body = "";
		status = Status::UNPARSED;
	}
};

class Response
{
public:
	Version version;
	int status_code;
	string code_description;
	map<string, string> header;
	string body;
	Response()
	{
		version = Version();
		status_code = 200;
		code_description = "200 OK";
		body = "";
	}
};

int parse_request(Connection&);

#endif
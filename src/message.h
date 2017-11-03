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
	Version() : major(1), minor(0) {}
};

class Request
{
public:
	Method method;
	Url url;
	Version version;
	map<Str, Str> header;
	Str body;
	Request()
	{
		method = Method::GET;
		url = Url();
		version = Version();
		body = Str();
		header = map<Str, Str>();
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
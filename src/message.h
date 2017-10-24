#ifndef ABYSS_REQUEST_H
#define ABYSS_REQUEST_H

#include<iostream>
#include<string>
#include<map>
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

	}
};

class Version
{
public:
	int major;
	int minor;
	Version() : major(0), minor(0) {}
};

class Request
{
public:
	Method method;
	Url url;
	Version version;
	map<string, string> header;
	string body;
};

class Response
{
public:
	Version version;
	int statue_code;
	string code_description;
	map<string, string> header;
	string body;
};

int parse_request(char*);

#endif
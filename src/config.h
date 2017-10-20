#ifndef ABYSS_CONGFIG_H
#define ABYSS_CONGFIG_H

#include"json.h"
#include<string>
using namespace gao;

class Config
{
public:
	int port;
	bool daemon;
	int worker;
	int timeout;
	int src_root;
	int err_root;
	Config() : port(0), daemon(false), worker(0), timeout(0), src_root(0), err_root(0) {}
};

int load_config(Config&, string);

#endif
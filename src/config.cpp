#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<cstring>
#include<string>
#include<errno.h>

#include"json.h"
#include"config.h"
#include"util.h"

using namespace gao;
using namespace std;

int load_config(Config& config, const char* filename)
{
	int fd = open(filename, O_RDONLY);
	if(fd == -1)
	{
		ABYSS_ERR_MSG(strerror(errno));
		return ABYSS_ERR;
	}
	struct stat buf;
	int ret = fstat(fd, &buf);
	if(ret == -1)
	{
		ABYSS_ERR_MSG(strerror(errno));
		return ABYSS_ERR;
	}

	char *cfg = new char[buf.st_size + 1];
	if(read(fd, cfg, buf.st_size) == -1)
	{
		delete cfg;
		close(fd);
		ABYSS_ERR_MSG(strerror(errno));
		return ABYSS_ERR;
	}
	cfg[buf.st_size] = '\0';
	JsonValue cfg_json;
	loads(cfg_json, string(cfg));
	if(cfg_json.JSONType() != Type::JSON_OBJECT)
	{
		ABYSS_ERR_MSG("json file load failed");
		return ABYSS_ERR;
	}

	if(!cfg_json.hasKey("port") || cfg_json["port"].JSONType() != Type::JSON_INTEGRAL)
	{
		ABYSS_ERR_MSG("Not sepcified port or wrong type");
		return ABYSS_ERR;
	}
	else if((config.port = cfg_json["port"].getNumber()) > 65535)
	{
		ABYSS_ERR_MSG("port specified greater than 65535");
		return ABYSS_ERR;
	}

	if(!cfg_json.hasKey("daemon") || cfg_json["daemon"].JSONType() != Type::JSON_BOOL)
	{
		ABYSS_ERR_MSG("Not sepcified daemon or wrong type");
		return ABYSS_ERR;
	}
	config.daemon = cfg_json["daemon"].getBoolean();

	if(!cfg_json.hasKey("worker") || cfg_json["worker"].JSONType() != Type::JSON_INTEGRAL)
	{
		ABYSS_ERR_MSG("Not sepcified worker or wrong type");
		return ABYSS_ERR;
	}
	else if((config.worker = cfg_json["worker"].getNumber()) > sysconf(_SC_NPROCESSORS_ONLN))
	{
		ABYSS_ERR_MSG("worker specified more than cpu cores");
		return ABYSS_ERR;
	}

	if(!cfg_json.hasKey("timeout") || cfg_json["timeout"].JSONType() != Type::JSON_INTEGRAL)
	{
		ABYSS_ERR_MSG("Not sepcified timeout or wrong type");
		return ABYSS_ERR;
	}
	config.timeout = cfg_json["timeout"].getNumber();

	if(!cfg_json.hasKey("src_root") || cfg_json["src_root"].JSONType() != Type::JSON_STRING)
	{
		ABYSS_ERR_MSG("Not sepcified src_root or wrong type");
		return ABYSS_ERR;
	}
	config.src_root = open(cfg_json["port"].getString().c_str(), O_RDONLY);
	if(config.src_root < 0)
	{
		ABYSS_ERR_MSG(strerror(errno));
		return ABYSS_ERR;
	}
	struct stat st;
	fstat(config.src_root, &st);
	if(!S_ISDIR(st.st_mode))
	{
		ABYSS_ERR_MSG("src_root not directory");
		return ABYSS_ERR;
	}
	
	if(!cfg_json.hasKey("err_root") || cfg_json["err_root"].JSONType() != Type::JSON_STRING)
	{
		ABYSS_ERR_MSG("Not sepcified err_root or wrong type");
		return ABYSS_ERR;
	}
	config.err_root = open(cfg_json["err_root"].getString().c_str(), O_RDONLY);
	if(config.err_root < 0)
	{
		ABYSS_ERR_MSG(strerror(errno));
		return ABYSS_ERR;
	}
	fstat(config.err_root, &st);
	if(!S_ISDIR(st.st_mode))
	{
		ABYSS_ERR_MSG("err_root not directory");
		return ABYSS_ERR;
	}

	return ABYSS_OK;
}

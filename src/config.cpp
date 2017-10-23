#include<stdio.h>
#include<stdlib.h>
#include<string>
#include"json.h"
#include"config.h"

using namespace gao;

int load_config(Config& config, string filename)
{
	FILE* config_file = fopen(filename, "r");

	string  = read(config_file);

	json
}

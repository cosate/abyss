#ifndef ABYSS_UTIL_H
#define ABYSS_UTIL_H

#include<stdio.h>
#include<stdlib.h>
#include<string>

#define ABYSS_ERR (-1)
#define ABYSS_OK (0)

#define ABYSS_ERR_MSG(msg) \
	fprintf(stderr, "[Exit in %s line %d]: %s\n", __FILE__, __LINE__, msg)

void abyss_log(const char* fmt, ...);

#endif
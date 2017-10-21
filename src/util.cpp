#include<stdarg.h>
#include<time.h>
#include<sys/types.h>
#include<unistd.h>
#include"util.h"


void abyss_log(const char* format, ...)
{
	FILE *log_file = fopen("log/Abyss.log", "a+");
	if(!log_file)
		return;

	time_t t = time(NULL);
	tm *time = localtime(&t);

	fprintf(log_file, )
}
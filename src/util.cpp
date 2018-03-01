#include<stdarg.h>
#include<time.h>
#include<sys/types.h>
#include<unistd.h>
#include"util.h"

void abyss_log(const char* fmt, ...)
{
	FILE *log_file = fopen("log/abyss.log", "a+");
	if(!log_file)
		return;

	time_t t = time(NULL);
	tm *time = localtime(&t);

	fprintf(log_file, "[%4d: %02d: %02d: %02d:%02d:%02d] [pid: %5d] ", 
		time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
		time->tm_hour, time->tm_min, time->tm_sec, getpid());

	va_list ap;
	va_start(ap, fmt);
	vfprintf(log_file, fmt, ap);
	va_end(ap);
	fprintf(log_file, "\n");
	fclose(log_file);
}
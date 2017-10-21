#include<stdio.h>
#include<netdb.h>
#include<cstring>
#include<iostream>
#include<string>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<linux/tcp.h>
#include<arpa/inet.h>
using namespace std;

int main(int argc, char* argv[])
{

	addrinfo hints, *res, *res_save;
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char port[6];
	snprintf(port, 6, "%d", 8080);

	char ips[16];
	int ret = getaddrinfo(NULL, port, &hints, &res);
	
	for(res_save = res; res != NULL; res = res->ai_next)
	{
		//inet_ntop(AF_INET, &(((struct sockaddr_in *)(res->ai_addr))->sin_addr), ips, 16);
		printf("%d\n", ntohs(((struct sockaddr_in *)(res->ai_addr))->sin_port));
	}

	freeaddrinfo(res_save);
	return 0;
}
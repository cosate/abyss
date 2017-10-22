#include<iostream>
#include<string>
#include<unistd.h>
#include<errno.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/epoll.h>

#include<vector>

#include"config.h"
#include"net.h"
#include"config.h"
#include"connection.h"

using namespace std;

vector<Connection*> connections;
Config config;
int epfd;

int main()
{
	epfd = epoll_create1(0);
	
	int listen_fd = creat_listen_socket(NULL, config.port, 1024);
	return 0;
}
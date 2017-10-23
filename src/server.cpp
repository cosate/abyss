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
#include<algorithm>

#include"config.h"
#include"net.h"
#include"config.h"
#include"connection.h"

using namespace std;

vector<Connection*> connections;
Config config;
int epfd;

void server_init()
{
	load_config(config, "config.json");
}


int main(int argc, char* argv[])
{
	server_init();
	make_heap(connections.begin(), connections.end(), cmp);
	epfd = epoll_create1(0);
	
	int listen_fd = creat_listen_socket(NULL, config.port, 1024);
	if(listen_fd == ABYSS_ERR)
	{
		ABYSS_ERR_MSG(strerror(errno));
		exit(-1);
	}

	EventData listen_data;
	listen_data.ptr = &listen_fd;
	listen_data.in_handler = accept_connection;

	epoll_event listen_event;
	listen_event.events = EPOLLIN;
	listen_event.data.ptr = &listen_data;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &listen_event) == -1)
	{
		ABYSS_ERR_MSG(strerror(errno));
		exit(-1);
	}

	epoll_event res[MAX_EVENTS];
	while(1)
	{
		int nfds = epoll_wait(epfd, res, MAX_EVENTS, 500);
		for(int i = 0; i < nfds; i++)
		{
			epoll_event ev = res[i];
			if(ev->evets & EPOLLIN)
		}
	}
	return 0;
}
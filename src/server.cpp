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

vector<EventData*> connections;
Config server_config;
int epfd;

void server_init()
{
	load_config(config, "config.json");
	make_heap(connections.begin(), connections.end(), cmp);
	epfd = epoll_create1(0);
}


int main(int argc, char* argv[])
{
	server_init();
	
	int listen_fd = creat_listen_socket(NULL, config.port, 1024);
	if(listen_fd == ABYSS_ERR)
	{
		ABYSS_ERR_MSG(strerror(errno));
		exit(ABYSS_ERR);
	}

	EventData* listen_data = new ListenData(listen_fd);

	epoll_event listen_event;
	listen_event.events = EPOLLIN;
	listen_event.data.ptr = listen_data;

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
			EventData* p = res[i].data.ptr;
			if(p->fd == listen_fd)
			{
				p->in_handler();
			}
			else
			{
				if(res[i].events & EPOLLIN)
				{
					if(p->in_handler() == ABYSS_ERR)
					{
						if(p->fd != -1)
							close(p->fd);
						p->active_time = 0;
						make_heap(connections.begin(), connections.end(), cmp);
						pop_heap(connections.begin(), connections.end(), cmp);
						connections.pop_back();
						delete p;
						memset(&res[i], 0, sizeof(res[i]));
					}
					else
					{
						p->active_time = time(NULL);
						make_heap(connections.begin(), connections.end(), cmp);
					}
				}
				if(res[i].events & EPOLLOUT)
				{
					if(p->out_handler() == ABYSS_ERR)
					{
						if(p->fd != -1)
							close(p->fd);
						p->active_time = 0;
						make_heap(connections.begin(), connections.end(), cmp);
						pop_heap(connections.begin(), connections.end(), cmp);
						connections.pop_back();
						delete p;
						memset(&res[i], 0, sizeof(res[i]));
					}
					else
					{
						p->active_time = time(NULL);
						make_heap(connections.begin(), connections.end(), cmp);
					}
				}
			}
		}
	}
	return 0;
}
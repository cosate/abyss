#include<iostream>
#include<string>
#include<unistd.h>
#include<errno.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/epoll.h>
#include<cstring>

#include<vector>
#include<algorithm>

#include"config.h"
#include"net.h"
#include"config.h"
#include"connection.h"
#include"util.h"

using namespace std;

vector<ConnectionData*> connections;
Config server_config;
int epfd;

void server_init()
{
	load_config(server_config, "config.json");
	make_heap(connections.begin(), connections.end(), cmp);
	epfd = epoll_create1(0);
	RequestHeader::init_field_position();
	Response::init_code_description();
	Response::init_mime();
}


int main(int argc, char* argv[])
{
	server_init();
	
	int listen_fd = create_listen_socket(NULL, server_config.port, 1024);
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
			EventData* p = (EventData*)(res[i].data.ptr);
			if(p->fd == listen_fd)
			{
				p->in_handler();
			}
			else
			{
				if(res[i].events & EPOLLIN)
				{
					if(((ConnectionData*)p)->in_handler() == ABYSS_ERR)
					{
						cout<<((ConnectionData*)p)->recv_buffer<<endl;
						((ConnectionData*)p)->request.print();
						if(((ConnectionData*)p)->fd != -1)
							close(((ConnectionData*)p)->fd);
						((ConnectionData*)p)->active_time = 0;
						make_heap(connections.begin(), connections.end(), cmp);
						pop_heap(connections.begin(), connections.end(), cmp);
						connections.pop_back();
						delete p;
						memset(&res[i], 0, sizeof(res[i]));
					}
					else
					{
						cout<<((ConnectionData*)p)->recv_buffer<<endl;
						((ConnectionData*)p)->request.print();
						((ConnectionData*)p)->active_time = time(NULL);
						make_heap(connections.begin(), connections.end(), cmp);
					}
				}
				if(res[i].events & EPOLLOUT)
				{
					if(((ConnectionData*)p)->out_handler() == ABYSS_ERR)
					{
						if(((ConnectionData*)p)->fd != -1)
							close(((ConnectionData*)p)->fd);
						((ConnectionData*)p)->active_time = 0;
						make_heap(connections.begin(), connections.end(), cmp);
						pop_heap(connections.begin(), connections.end(), cmp);
						connections.pop_back();
						delete p;
						memset(&res[i], 0, sizeof(res[i]));
					}
					else
					{
						((ConnectionData*)p)->active_time = time(NULL);
						make_heap(connections.begin(), connections.end(), cmp);
					}
				}
			}
		}
	}
	return 0;
}
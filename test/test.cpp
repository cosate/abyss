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
#include<sys/epoll.h>
#include<sys/time.h>
#include<sys/types.h>

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

	int listen_fd;
	
	for(res_save = res; res != NULL; res = res->ai_next)
	{
		listen_fd = socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK, res->ai_protocol);
		bind(listen_fd, res->ai_addr, res->ai_addrlen);
		listen(listen_fd, 1024);
		break;
	}

	freeaddrinfo(res_save);

	cout<<"listen fd "<<listen_fd<<endl;


	int epfd = epoll_create1(0);

	epoll_event listen_event;
	listen_event.events = EPOLLIN;
	listen_event.data.fd = listen_fd;

	epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &listen_event);

	epoll_event results[1024];
	char buff[4096];

	for(;;)
	{
		int nfds = epoll_wait(epfd, results, 1024, 500);
		cout<<"loop"<<endl;
		for(int i = 0; i < nfds; i++)
		{
			int res = results[i].data.fd;
			if(res == listen_fd)
			{
				cout<<"listen"<<endl;
				int connfd = accept(res, NULL, NULL);
				epoll_event bew;
				bew.events = EPOLLIN;
				bew.data.fd = connfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &bew);
			}
			else
			{
				cout<<"res "<<res<<endl;
				recv(res, buff, 4096, 0);
				cout<<buff<<endl;
			}
		}
	}
	return 0;
}
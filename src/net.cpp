#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/type.h>
#include<netdb.h>
#include<linux/tcp.h>

#include"net.h"
#include"util.h"

static int set_reuseport(int sockfd)
{
	int on = 1;
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
	if(ret == -1)
	{
		ABYSS_ERR_MSG(strerr(errno));
		return ABYSS_ERR;
	}
	return ABYSS_OK;
}
int create_listen_socket(const char* host, int port, int backlog)
{
	int listen_fd;

	char _port[6];
	snprintf(_port, 6, ,"%d", port);

	addrinfo hints, *results, *results_save;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int ret = getaddrinfo(host, _port, &hints, &results);
	if(ret != 0)
	{
		ABYSS_ERR_MSG(gai_strerror(ret));
		return ABYSS_ERR;
	}
	else
	{
		for(results_save = res; res != NULL; res = res->ai_next)
		{
			listen_fd = socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK, res->ai_protocol);
			if(listen_fd == -1)
				continue;

			if(set_reuseport(listen_fd) == ABYSS_ERR)
			{
				close(listen_fd);
				continue;
			}

			if(bind(listen_fd, res->ai_addr, res->ai_addrlen) == -1)
			{
				close(listen_fd);
				continue;
			}

			if(listen(listen, backlog) == -1)
			{
				close(listen_fd);
				continue;
			}

			break;
		}

		freeaddrinfo(results_save);

		return results == NULL ? ABYSS_ERR : listen_fd;
	}
}
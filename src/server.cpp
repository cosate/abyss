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
#include<sys/resource.h>

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

static void print_usage()
{
	printf("Usage : abyss [option]\n"
		   "    --stop    Stop abyss.\n"
		   "    --restart Restart abyss and reload config.json.\n"
		   "    --help    Print usage.\n");
}

static pid_t get_pid()
{
	FILE * fp = fopen("log/abyss.pid", "r");
	if(!fp)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "open pid file: log/abyss.pid failed");
		exit(-1);
	}
	pid_t pid = 0;
	fscanf(fp, "%d", &pid);
	fclose(fp);
	return pid;
}

static void save_pid(pid_t pid)
{
	FILE *fp = fopen("log/abyss.pid", "w");
	if(!fp)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "open pid file: log/abyss.pid failed");
		exit(-1);
	}
	fprintf(fp, "%d", pid);
	fclose(fp);
}

static void send_signal(int sig)
{
	pid_t pid = get_pid();
	if(pid == 0)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "abyss is not running");
		exit(-1);
	}
	kill(pid, sig);
}

enum class abyss_status {ABYSS_STOP = 0, ABYSS_RELOAD};
static abyss_status status = abyss_status::ABYSS_STOP;

static void sigint_handler(int sig)
{
	abyss_log("abyss exited");
	switch(status)
	{
		case abyss_status::ABYSS_STOP:
		{
			kill(-get_pid(), SIGINT);
			save_pid(0);
			raise(SIGKILL);
			break;
		}
		case abyss_status::ABYSS_RELOAD:
		{
			status = abyss_status::ABYSS_STOP;
			break;
		}
		default:
		{
			fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "invalid reload_flag in sigint_handler");
			exit(-1);
		}
	}
}

static void sighup_handler(int sig)
{
	abyss_log("abyss reload config.json and restart");
	close(server_config.src_root);
	close(server_config.err_root);
	if(load_config(server_config, "config.json") != ABYSS_OK)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "load config.json failed");
		exit(-1);
	}
	status = abyss_status::ABYSS_RELOAD;
	kill(-get_pid(), SIGINT);
}

static void set_sig_handler(int sig, void(*handler)(int))
{
	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	if(sigaction(sig, &sa, NULL) == -1)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "set signal handler failed");
		exit(-1);
	}
}

static void server_init()
{
	if(load_config(server_config, "config.json") != ABYSS_OK)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "load config.json failed");
		exit(-1);
	}
	signal(SIGPIPE, SIG_IGN);
	set_sig_handler(SIGINT, sigint_handler);
	set_sig_handler(SIGTERM, sigint_handler);
	set_sig_handler(SIGHUP, sighup_handler);
	struct rlimit nofile_limit = { 65535, 65535 };
	if(setrlimit(RLIMIT_NOFILE, &nofile_limit) == -1)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, strerror(errno));
		exit(-1);
	}

	make_heap(connections.begin(), connections.end(), cmp);
	epfd = epoll_create1(0);
	RequestHeader::init_field_position();
	Response::init_code_description();
	Response::init_mime();

	if(server_config.daemon)
		daemon(1, 0);

	save_pid(getpid());
}

int main(int argc, char* argv[])
{
	if(argc >= 2)
	{
		if(!strncmp(argv[1], "--stop", 6))
			send_signal(SIGINT);
		else if(!strncmp(argv[1], "--restart", 9))
			send_signal(SIGHUP);
		else
			print_usage();
		exit(0);
	}

	if(get_pid() != 0)
	{
		fprintf(stderr, "[EXIT in %s:%d] %s\n", __FILE__, __LINE__, "abyss is running");
		exit(-1);
	}
	get_pid();
	server_init();
	abyss_log("abyss server started, listening at port: %u", server_config.port);
	
	int worker = 0;
	for(;;)
	{
		if(worker >= server_config.worker)
		{
			wait(NULL);
			worker--;
			abyss_log("abyss subprocess failed, restarting");
		}

		pid_t pid = fork();
		if(pid == 0)
			break;
		else if(pid < 0)
			continue;
		worker++;
	}

	signal(SIGINT, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	
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
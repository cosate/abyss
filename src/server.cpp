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
#include"connection.h"

using namespace std;

vector<Connection*>;
int epfd;

int main()
{
	epfd = epoll_create1(0);
	
	return 0;
}
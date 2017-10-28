class EventData
{
public:
	int fd;
	virtual int in_handler() = 0;
	EventData() : fd(-1) {}
	EventData(int f) :fd(f) {}
};

class ConnectionData : public EventData
{
public:
	time_t active_time;
	Request request;
	Response response;
	char send_buffer[BUFFERSIZE];
	char recv_buffer[BUFFERSIZE];
	int buffer_length;
	Connection() : EventData()
	{
		construct();
	}

	Connection(int f) : EventData(f)
	{
		construct();
	}

	int in_handler();
	int out_handler();
private:
	void construct();
};

void ConnectionData::construct()
{
	request = Request();
	response = Response();
	memset(send_buffer, 0, BUFFERSIZE);
	memset(recv_buffer, 0, BUFFERSIZE);
	buffer_length = 0;
}

int ConnectionData::in_handler()
{

}

int ConnectionData::out_handler()
{

}

class ListenData : public EventData
{
public:
	ListenData() : EventData() {}
	ListenData(int f) : EventData(f) {}

	int in_handler();
}

int EventData::in_handler()
{
	int connfd;
	while((connfd = accept(this->fd, NULL, NULL)) != -1)
	{
		if(connections.size() >= MAX_CONNECTIONS)
		{
			close(connfd);
			continue;
		}
		if(set_nonblock(connfd) == ABYSS_ERR)
		{
			close(connfd);
			continue;
		}
		EventData* c = new ConnectionData(connfd);
		
		epoll_event connev;
		connev.events = EPOLLIN;
		connev.data.ptr = c;
		
		if(epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &connev) == ABYSS_ERR)
		{
			close(connfd);
			delete c;
			continue;
		}
		c->activae_time = time(NULL);
		connections.push_back(c);
		push_heap(connections.begin(), connections.end(), cmp);
	}

	if(errno != EWOULDBLOCK)
	{
		ABYSS_ERR_MSG(strerror(errno));
		return ABYSS_ERR;
	}
	return ABYSS_OK;
}
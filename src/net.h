#ifndef ABYSS_NET_H
#define ABYSS_NET_H

int set_nonblock(int);
int set_tcp_cork(int);
int reset_tcp_cork(int);
int create_listen_socket(const char*, int, int);

#endif
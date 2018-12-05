#ifndef _UDTCP_H_
# define _UDTCP_H_

int     udtcp_connection(const char *host, int port, int *ret_sock_fd);
int     udtcp_get_udp_port(int sock);

#endif
#include "udtcp.h"

void udtcp_tcp_server_poll_break_loop(udtcp_tcp_server_t *in_server)
{
    in_server->poll_loop = 0;
}

void udtcp_tcp_server_poll_loop(udtcp_tcp_server_t *in_server, long timeout)
{
    in_server->poll_loop = 1;
    while (in_server->poll_loop && udtcp_tcp_server_poll(in_server, timeout) >= 0);
}
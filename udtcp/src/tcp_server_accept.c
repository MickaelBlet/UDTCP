#include "udtcp.h"

int udtcp_tcp_server_accept(udtcp_tcp_server_t *in_server, udtcp_tcp_infos_t **out_client, long timeout)
{
    socklen_t           len;
    struct pollfd       poll_fd;
    int                 ret_poll;
    int                 sock_opt;
    socklen_t           socklen_opt;

    if (udtcp_noblock_socket(in_server->server_infos->socket, 0) == -1)
    {
        UDTCP_LOG(in_server, 2, "fcntl() fail");
        return (-1);
    }

    /* prepare poll */
    poll_fd.fd = in_server->server_infos->socket;
    poll_fd.events = POLLIN;
    ret_poll = poll(&poll_fd, 1, timeout);
    if (ret_poll < 0)
    {
        UDTCP_LOG(in_server, 2, "poll() fail");
        return (-1);
    }
    else if (ret_poll > 0)
    {
        socklen_opt = sizeof(int);
        if (getsockopt(in_server->server_infos->socket, SOL_SOCKET, SO_ERROR, (void*)(&sock_opt), &socklen_opt) < 0)
        {
            UDTCP_LOG(in_server, 2, "getsockopt(): failed");
            return (-1);
        }
        if (sock_opt)
        {
            UDTCP_LOG(in_server, 2, "poll(): delay");
            return (-1);
        }
    }
    else
    {
        UDTCP_LOG(in_server, 2, "poll(): timeout");
        return (-1);
    }

    len = sizeof(struct sockaddr_in);
    in_server->clients_infos[in_server->poll_nfds]->socket = accept( \
        in_server->server_infos->socket, \
        (struct sockaddr*)&(in_server->clients_infos[in_server->poll_nfds]->addr), \
        &len);
    if (in_server->clients_infos[in_server->poll_nfds]->socket == -1)
    {
        UDTCP_LOG(in_server, 2, "accept(): failed");
        return -1;
    }

    if (udtcp_noblock_socket(in_server->server_infos->socket, 1) == -1)
    {
        UDTCP_LOG(in_server, 2, "fcntl() fail");
        return (-1);
    }

    /* copy infos */
    in_server->clients_infos[in_server->poll_nfds]->id = in_server->count_id;
    const char *hostname = gethostbyaddr( \
        &(in_server->clients_infos[in_server->poll_nfds]->addr.sin_addr), \
        sizeof(struct in_addr), AF_INET)->h_name;
    memcpy(in_server->clients_infos[in_server->poll_nfds]->hostname, hostname, strlen(hostname));
    const char *ip = inet_ntoa(in_server->clients_infos[in_server->poll_nfds]->addr.sin_addr);
    memcpy(in_server->clients_infos[in_server->poll_nfds]->ip, ip, strlen(ip));

    ++in_server->count_id;

    /* add in poll table */
    in_server->poll_fds[in_server->poll_nfds].fd = in_server->clients_infos[in_server->poll_nfds]->socket;
    in_server->poll_fds[in_server->poll_nfds].events = POLLIN;

    UDTCP_LOG(in_server, 0, "connect [%lu]%s:%d", \
        in_server->clients_infos[in_server->poll_nfds]->id, \
        in_server->clients_infos[in_server->poll_nfds]->hostname, \
        in_server->clients_infos[in_server->poll_nfds]->port);

    if (in_server->connect_callback != NULL)
        in_server->connect_callback(in_server->user_data, in_server->clients_infos[in_server->poll_nfds]);

    *out_client = in_server->clients_infos[in_server->poll_nfds];

    ++in_server->poll_nfds;

    return 0;
}
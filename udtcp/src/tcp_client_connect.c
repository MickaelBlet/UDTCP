#include "udtcp.h"

static int timeout_poll(const udtcp_tcp_client_t *in_client, long timeout)
{
    struct pollfd  poll_fd;
    int            ret_poll;
    int            sock_opt;
    socklen_t      socklen_opt;

    if (errno == EINPROGRESS)
    {
        poll_fd.fd = in_client->client_infos.socket;
        poll_fd.events = POLLOUT;
        ret_poll = poll(&poll_fd, 1, timeout);
        if (ret_poll < 0)
        {
            UDTCP_LOG(in_client, UDTCP_LOG_ERROR, "poll(): failed");
            return (-1);
        }
        else if (ret_poll > 0)
        {
            socklen_opt = sizeof(int);
            if (getsockopt(in_client->client_infos.socket, SOL_SOCKET, SO_ERROR, (void*)(&sock_opt), &socklen_opt) < 0)
            {
                UDTCP_LOG(in_client, UDTCP_LOG_ERROR, "getsockopt(): %s", strerror(errno));
                return (-1);
            }
            if (sock_opt)
            {
                // fprintf(stderr, "Error in delayed connection() %d - %s\n", sock_opt, strerror(sock_opt));
                in_client->log_callback(in_client->user_data, 2, "delay connect()");
                return (-1);
            }
        }
        else
        {
            in_client->log_callback(in_client->user_data, 1, "connect(): timeout");
            errno = ETIMEDOUT;
            return (-1);
        }
    }
    else if (errno > 0)
    {
        in_client->log_callback(in_client->user_data, 2, "connect(): failed");
        return (-1);
    }
    return 0;
}

int udtcp_tcp_client_connect(udtcp_tcp_client_t *in_out_client, const char *in_hostname, uint16_t port, long timeout)
{
    struct hostent     *host_entity;

    /* get host list by name */
    host_entity = gethostbyname(in_hostname);
    if (host_entity == NULL)
    {
        UDTCP_LOG(in_out_client, UDTCP_LOG_ERROR, "gethostbyname(\"%s\") failed", in_hostname);
        return (-1);
    }

    if (udtcp_noblock_socket(in_out_client->client_infos.socket, 0) == -1)
    {
        UDTCP_LOG(in_out_client, UDTCP_LOG_ERROR, "fcntl() failed");
        return (-1);
    }

    memset(&(in_out_client->server_infos.addr), 0, sizeof(struct sockaddr_in));

    /* copy the first address in hostname list */
    memcpy(&in_out_client->server_infos.addr.sin_addr, host_entity->h_addr_list[0], host_entity->h_length);
    in_out_client->server_infos.addr.sin_family = AF_INET;
    in_out_client->server_infos.addr.sin_port = htons(port);

    if (connect(in_out_client->client_infos.socket, (struct sockaddr*)&(in_out_client->server_infos.addr), sizeof(struct sockaddr_in)) < 0)
    {
        if (timeout_poll(in_out_client, timeout) == -1)
        {
            shutdown(in_out_client->client_infos.socket, SHUT_RDWR);
            close(in_out_client->client_infos.socket);
            return (-1);
        }
    }

    if (udtcp_noblock_socket(in_out_client->client_infos.socket, 1) == -1)
    {
        UDTCP_LOG(in_out_client, UDTCP_LOG_ERROR, "fcntl() failed");
        return (-1);
    }

    /* copy infos */
    in_out_client->server_infos.id = 0;
    in_out_client->server_infos.socket = in_out_client->client_infos.socket;
    const char *hostname = gethostbyaddr(&(in_out_client->server_infos.addr.sin_addr), sizeof(struct in_addr), AF_INET)->h_name;
    memcpy(in_out_client->server_infos.hostname, hostname, strlen(hostname));
    const char *ip = inet_ntoa(in_out_client->server_infos.addr.sin_addr);
    memcpy(in_out_client->server_infos.ip, ip, strlen(ip));

    if (in_out_client->connect_callback != NULL)
            in_out_client->connect_callback(in_out_client, &(in_out_client->server_infos));

    return (0);
}
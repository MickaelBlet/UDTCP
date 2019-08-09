#include "udtcp.h"

enum e_receive udtcp_tcp_receive(udtcp_tcp_client_t *in_client, void **out_data, size_t *out_data_size, long timeout)
{
    struct pollfd   poll_fd;
    int             ret_poll;
    int             sock_opt;
    socklen_t       socklen_opt;
    // int             ret_recv;

    if (udtcp_noblock_socket(in_client->server_infos.socket, 0) == -1)
    {
        UDTCP_LOG(in_client, 2, "fcntl() failed");
        return (-1);
    }

    poll_fd.fd = in_client->server_infos.socket;
    poll_fd.events = POLLIN;
    ret_poll = poll(&poll_fd, 1, timeout);
    if (ret_poll < 0)
    {
        in_client->log_callback(in_client, 2, "connect(): failed");
        return (UDTCP_RECEIVE_ERROR);
    }
    else if (ret_poll > 0)
    {
        socklen_opt = sizeof(int);
        if (getsockopt(in_client->server_infos.socket, SOL_SOCKET, SO_ERROR, (void*)(&sock_opt), &socklen_opt) < 0)
        {
            in_client->log_callback(in_client, 2, "getsockopt(): failed");
            return (UDTCP_RECEIVE_ERROR);
        }
        if (sock_opt)
        {
            in_client->log_callback(in_client, 2, "connect(): delay");
            return (UDTCP_RECEIVE_ERROR);
        }
    }
    else
    {
        in_client->log_callback(in_client, 1, "connect(): timeout");
        return (UDTCP_RECEIVE_TIMEOUT);
    }

    void*       data;
    uint32_t    data_size;
    int         ret_recv;

    // ret_recv = recv(in_client->server_infos.socket, &data_size, sizeof(uint32_t), 0);
    // if (ret_recv < 0)
    // {
    //     return (-1);
    // }
    // if (ret_recv == 0)
    // {
    //     UDTCP_LOG(in_client, 0, "recv(): close [%lu]%s:%d\n", \
    //         in_client->client_infos.id, \
    //         in_client->client_infos.hostname, \
    //         in_client->client_infos.port);
    //     return (0);
    // }

    // if (ret_recv != sizeof(uint32_t))
    // {
    //     UDTCP_LOG(in_client, 2, "recv(): fail [%lu]%s:%d\n", \
    //         in_client->client_infos.id, \
    //         in_client->client_infos.hostname, \
    //         in_client->client_infos.port);
    //     return (-1);
    // }

    data = malloc(4096);
    if (data == NULL)
    {
        UDTCP_LOG(in_client, 2, "malloc(): fail\n");
        return (-1);
    }

    ret_recv = recv(in_client->server_infos.socket, data, 4096, 0);
    // if (ret_recv != (int)(data_size))
    // {
    //     free(data);
    //     UDTCP_LOG(in_client, 2, "recv(): fail [%lu]%s:%u\n", \
    //         in_client->client_infos.id, \
    //         in_client->client_infos.hostname, \
    //         in_client->client_infos.port);
    //     return (-1);
    // }

    if (in_client->receive_callback != NULL)
    {
        in_client->receive_callback(in_client, &in_client->server_infos, data, data_size);
        free(data);
    }

    if (udtcp_noblock_socket(in_client->server_infos.socket, 1) == -1)
    {
        UDTCP_LOG(in_client, 0, "fcntl() failed\n");
        return (UDTCP_RECEIVE_FCNTL_ERROR);
    }

    *out_data = data;
    *out_data_size = ret_recv;

    return (UDTCP_RECEIVE_OK);
}
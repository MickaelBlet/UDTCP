#include <stdlib.h>     /* malloc, free */
#include <unistd.h>     /* close */
#include <string.h>     /* strerror */
#include <errno.h>      /* errno */

#include "udtcp.h"

static int receive_tcp(udtcp_client* client, udtcp_infos* server_infos)
{
    ssize_t         ret_recv;
    uint8_t*        new_buffer_data;
    uint32_t        buffer_size = 0;
    uint32_t        data_size;
    struct pollfd   poll_fd;
    int             ret_poll;

    /* receive data size */
    ret_recv = recv(server_infos->tcp_socket,
                    &data_size, sizeof(uint32_t), 0);
    /* recv error */
    if (ret_recv < 0)
        return (-1);
    /* recv close */
    if (ret_recv == 0)
        return (0);

    /* not a size of data */
    if (ret_recv != sizeof(uint32_t))
    {
        UDTCP_LOG_ERROR(client,
            "Receive from %s:%hu: bad size",
            server_infos->ip,
            server_infos->tcp_port);
        return (-1);
    }

    /* need reallocate buffer */
    if (data_size > client->buffer_size)
    {
        new_buffer_data = (uint8_t*)malloc(data_size * sizeof(uint8_t));
        /* malloc error */
        if (new_buffer_data == NULL)
        {
            UDTCP_LOG_ERROR(client,
                "malloc: %s", strerror(errno));
            return (-1);
        }
        /* delete last buffer */
        if (client->buffer_data != NULL)
            free(client->buffer_data);
        /* set new buffer */
        client->buffer_data = new_buffer_data;
        client->buffer_size = data_size;
    }

    do
    {
        poll_fd.fd = server_infos->tcp_socket;
        poll_fd.events = POLLIN;
        ret_poll = poll(&poll_fd, 1, 60 * 3 * 1000);
        if (ret_poll < 0)
        {
            UDTCP_LOG_ERROR(client, "poll(): failed");
            return (-1);
        }
        else if (ret_poll == 0)
        {
            UDTCP_LOG_ERROR(client, "poll(): timeout");
            return (-1);
        }
        /* receive data */
        ret_recv = recv(server_infos->tcp_socket,
                        client->buffer_data + buffer_size,
                        data_size - buffer_size, 0);
        /* recv error */
        if (ret_recv < 0)
            return (-1);
        /* recv close */
        if (ret_recv == 0)
            return (0);
        buffer_size += ret_recv;
    } while (buffer_size != data_size);

    if (client->receive_tcp_callback != NULL)
    {
        client->receive_tcp_callback(client, server_infos,
            client->buffer_data,
            data_size);
    }

    return (1);
}

static int receive_udp(udtcp_client* client, udtcp_infos* server_infos)
{
    socklen_t           len_addr = sizeof(struct sockaddr_in);
    uint8_t*            new_buffer_data;
    uint32_t            data_size;
    int                 ret_recv;
    struct sockaddr_in  addr;

    ret_recv = recvfrom(client->client_infos->udp_server_socket,
        &data_size, sizeof(uint32_t),
        MSG_PEEK,
        (struct sockaddr*)&addr,
        &len_addr);
    if (ret_recv < 0)
        return (-1);
    if (ret_recv == 0)
        return (0);

    if (ret_recv != sizeof(uint32_t))
    {
        UDTCP_LOG_ERROR(client,
            "[%lu]%s:%hu UDP recv: bad size",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }

    if (data_size + sizeof(uint32_t) > client->buffer_size)
    {
        new_buffer_data = (uint8_t*)malloc(data_size + sizeof(uint32_t));
        if (new_buffer_data == NULL)
        {
            UDTCP_LOG_ERROR(client,
                "[%lu]%s:%hu TCP recv: size too big (%u)",
                server_infos->id,
                server_infos->hostname,
                server_infos->udp_server_port, data_size);
            return (-1);
        }
        if (client->buffer_data != NULL)
            free(client->buffer_data);
        client->buffer_data = new_buffer_data;
        client->buffer_size = data_size + sizeof(uint32_t);
    }

    ret_recv = recvfrom(client->client_infos->udp_server_socket,
        client->buffer_data, sizeof(uint32_t) + data_size, 0,
        (struct sockaddr*)&addr,
        &len_addr);
    if (ret_recv < 0)
    {
        UDTCP_LOG_ERROR(client,
            "[%lu]%s:%hu UDP recv: fail",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }
    if (ret_recv == 0)
    {
        UDTCP_LOG_INFO(client,
            "[%lu]%s:%hu UDP recv: close",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (0);
    }
    if (ret_recv - sizeof(uint32_t) != data_size)
    {
        UDTCP_LOG_ERROR(client,
            "[%lu]%s:%hu UDP recv: bad size",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }

    if (client->receive_udp_callback != NULL)
    {
        client->receive_udp_callback(client, server_infos,
            client->buffer_data + sizeof(uint32_t), data_size);
    }

    return (1);
}

static void disconnect(udtcp_client* client, udtcp_infos* server_infos)
{
    shutdown(client->client_infos->tcp_socket, SHUT_RDWR);
    close(client->client_infos->tcp_socket);
    client->client_infos->tcp_socket = -1;
    UDTCP_LOG_INFO(client,
        "Disconnect ip: %s, "
        "tcp port: %u, "
        "udp server port: %u, "
        "udp client port: %u",
        server_infos->ip,
        (unsigned int)server_infos->tcp_port,
        (unsigned int)server_infos->udp_server_port,
        (unsigned int)server_infos->udp_client_port);
    if (client->disconnect_callback != NULL)
        client->disconnect_callback(client, server_infos);
}

enum udtcp_poll_e udtcp_client_poll(udtcp_client* client, long timeout)
{
    size_t  i;
    int     ret_poll;
    int     ret_receive;

    ret_poll = poll(client->poll_fds, client->poll_nfds, timeout);
    if (ret_poll < 0)
    {
        if (errno == EINTR)
            return (UDTCP_POLL_SIGNAL);
        UDTCP_LOG_ERROR(client, "poll: fail \"%s\"", strerror(errno));
        return (UDTCP_POLL_ERROR);
    }
    if (ret_poll == 0)
    {
        UDTCP_LOG_INFO(client, "poll: timeout");
        return (UDTCP_POLL_TIMEOUT);
    }

    for (i = 0; i < client->poll_nfds; ++i)
    {
        /* not active fd */
        if (!client->poll_fds[i].revents)
            continue;

        if (!(client->poll_fds[i].revents & POLLIN))
            return (UDTCP_POLL_ERROR);

        /* is tcp socket */
        if (client->poll_fds[i].fd == client->client_infos->tcp_socket)
        {
            ret_receive = receive_tcp(client, client->server_infos);
            if (ret_receive == -1)
            {
                if (errno != EWOULDBLOCK)
                {
                    disconnect(client, client->server_infos);
                    return (UDTCP_POLL_ERROR);
                }
            }
            if (ret_receive == 0)
            {
                disconnect(client, client->server_infos);
                return (UDTCP_POLL_ERROR);
            }
        }
        /* is udp socket */
        else if (client->poll_fds[i].fd == client->client_infos->udp_server_socket)
        {
            while (receive_udp(client, client->server_infos) > 0);
        }
    }
    return (UDTCP_POLL_SUCCESS);
}
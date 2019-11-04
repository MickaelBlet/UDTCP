/**
 * udtcp
 *
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019 BLET Mickaël.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <arpa/inet.h>  /* inet_ntoa */
#include <errno.h>      /* errno */
#include <string.h>     /* strerror */
#include <unistd.h>     /* close */

#include "udtcp.h"
#include "udtcp_utils.h"

static int receive_tcp(udtcp_client* client, udtcp_infos* server_infos)
{
    struct pollfd   poll_fd;
    ssize_t         ret_recv;
    uint32_t        buffer_size;
    uint32_t        data_size;
    int             ret_poll;
    uint8_t*        new_buffer_data;

    /* receive data size */
    ret_recv = recv(server_infos->tcp_socket, &data_size, sizeof(uint32_t), 0);
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
            "Receive from TCP %s:%u: recv: bad size",
            server_infos->ip,
            (unsigned int)server_infos->tcp_port);
        return (-1);
    }

    /* need reallocate buffer */
    if (data_size > client->buffer_size)
    {
        new_buffer_data = udtcp_new_buffer(data_size);
        /* malloc error */
        if (new_buffer_data == NULL)
        {
            UDTCP_LOG_ERROR(client, "malloc: %s", strerror(errno));
            return (-1);
        }
        /* delete last buffer */
        if (client->buffer_data != NULL)
            udtcp_free_buffer(client->buffer_data);
        /* set new buffer */
        client->buffer_data = new_buffer_data;
        client->buffer_size = data_size;
    }

    /* receive data */
    ret_recv = recv(server_infos->tcp_socket,
        client->buffer_data,
        data_size, 0);
    /* recv error */
    if (ret_recv < 0)
        return (-1);
    /* recv close */
    if (ret_recv == 0)
        return (0);
    /* recv not receive all data */
    if (ret_recv != data_size)
    {
        buffer_size = ret_recv;
        while (buffer_size != data_size)
        {
            poll_fd.fd = server_infos->tcp_socket;
            poll_fd.events = POLLIN;
            /* wait action from tcp socket */
            ret_poll = poll(&poll_fd, 1, -1);
            if (ret_poll < 0)
            {
                if (errno == EINTR)
                    return (-1);
                UDTCP_LOG_ERROR(client,
                    "Receive from TCP %s:%u: poll: failed",
                    server_infos->ip,
                    (unsigned int)server_infos->tcp_port);
                return (-1);
            }
            else if (ret_poll == 0)
            {
                UDTCP_LOG_ERROR(client,
                    "Receive from TCP %s:%u: poll: timeout",
                    server_infos->ip,
                    (unsigned int)server_infos->tcp_port);
                return (-1);
            }
            /* receive end of data */
            ret_recv = recv(server_infos->tcp_socket,
                client->buffer_data + buffer_size,
                data_size - buffer_size, 0);
            /* recv error */
            if (ret_recv < 0)
            {
                UDTCP_LOG_ERROR(client,
                    "Receive from TCP %s:%u: recv: error",
                    server_infos->ip,
                    (unsigned int)server_infos->tcp_port);
                return (-1);
            }
            /* recv close */
            if (ret_recv == 0)
                return (0);
            buffer_size += ret_recv;
        }
    }

    /* use callback if defined */
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

    /* receive data size */
    ret_recv = recvfrom(client->client_infos->udp_server_socket,
        &data_size, sizeof(uint32_t),
        MSG_PEEK,
        (struct sockaddr*)&addr,
        &len_addr);
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
            "Receive from UDP %s:%u: bad size",
            inet_ntoa(addr.sin_addr),
            (unsigned int)htons(addr.sin_port));
        return (-1);
    }

    /* need reallocate buffer */
    if (sizeof(uint32_t) + data_size > client->buffer_size)
    {
        new_buffer_data = udtcp_new_buffer(sizeof(uint32_t) + data_size);
        /* malloc error */
        if (new_buffer_data == NULL)
        {
            UDTCP_LOG_ERROR(client, "malloc: %s", strerror(errno));
            return (-1);
        }
        /* delete last buffer */
        if (client->buffer_data != NULL)
            udtcp_free_buffer(client->buffer_data);
        /* set new buffer */
        client->buffer_data = new_buffer_data;
        client->buffer_size = data_size + sizeof(uint32_t);
    }

    len_addr = sizeof(struct sockaddr_in);
    /* receive data_size and data */
    ret_recv = recvfrom(client->client_infos->udp_server_socket,
        client->buffer_data, sizeof(uint32_t) + data_size, 0,
        (struct sockaddr*)&addr,
        &len_addr);
    /* recv error */
    if (ret_recv < 0)
    {
        UDTCP_LOG_ERROR(client,
            "[%lu]%s:%hu UDP recv: fail",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }
    /* recv close */
    if (ret_recv == 0)
    {
        UDTCP_LOG_INFO(client,
            "[%lu]%s:%hu UDP recv: close",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (0);
    }
    /* recv bad size */
    if (ret_recv - sizeof(uint32_t) != data_size)
    {
        UDTCP_LOG_ERROR(client,
            "[%lu]%s:%hu UDP recv: bad size",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }

    /* use callback if defined */
    if (client->receive_udp_callback != NULL)
    {
        client->receive_udp_callback(client, server_infos,
            client->buffer_data + sizeof(uint32_t), data_size);
    }

    return (1);
}

static void disconnect(udtcp_client* client, udtcp_infos* server_infos)
{
    /* send '\0' to the server */
    if (shutdown(client->client_infos->tcp_socket, SHUT_RDWR) == -1)
    {
        UDTCP_LOG_ERROR(client, "shutdown: fail");
    }
    /* close tcp socket */
    if (close(client->client_infos->tcp_socket) == -1)
    {
        UDTCP_LOG_ERROR(client, "close: fail");
    }
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
    /* use callback if defined */
    if (client->disconnect_callback != NULL)
        client->disconnect_callback(client, server_infos);
}

__attribute__((weak))
enum udtcp_poll_e udtcp_client_poll(udtcp_client* client, long timeout)
{
    size_t  i;
    int     ret_poll;
    int     ret_receive;

    /* wait action from sockets */
    ret_poll = poll(client->poll_fds, client->poll_nfds, timeout);
    /* error or receive signal */
    if (ret_poll < 0)
    {
        if (errno == EINTR)
            return (UDTCP_POLL_SIGNAL);
        UDTCP_LOG_ERROR(client, "poll: fail \"%s\"", strerror(errno));
        return (UDTCP_POLL_ERROR);
    }
    /* timeout */
    if (ret_poll == 0)
    {
        UDTCP_LOG_INFO(client, "poll: timeout");
        return (UDTCP_POLL_TIMEOUT);
    }

    /* foreach socket */
    for (i = 0; client->poll_loop && i < client->poll_nfds; ++i)
    {
        /* not active fd */
        if (!client->poll_fds[i].revents)
            continue;

        /* bad revent */
        if (!(client->poll_fds[i].revents & POLLIN))
        {
            UDTCP_LOG_ERROR(client, "poll: revent failed");
            return (UDTCP_POLL_ERROR);
        }

        /* is tcp socket */
        if (client->poll_fds[i].fd == client->client_infos->tcp_socket)
        {
            ret_receive = receive_tcp(client, client->server_infos);
            while (client->poll_loop && ret_receive > 0)
                ret_receive = receive_tcp(client, client->server_infos);
            /* error socket */
            if (ret_receive < 0)
            {
                /* not nonblock errno */
                if (errno != EWOULDBLOCK)
                {
                    disconnect(client, client->server_infos);
                    return (UDTCP_POLL_ERROR);
                }
            }
            /* close socket */
            else if (ret_receive == 0)
            {
                disconnect(client, client->server_infos);
                return (UDTCP_POLL_ERROR);
            }
        }
        /* is udp socket */
        else if (client->poll_fds[i].fd == client->client_infos->udp_server_socket)
        {
            while (client->poll_loop
                    && receive_udp(client, client->server_infos) > 0);
        }
    }
    return (UDTCP_POLL_SUCCESS);
}
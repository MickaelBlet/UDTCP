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

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "udtcp.h"
#include "udtcp_utils.h"

static int initialize_connection(udtcp_server* server,
    udtcp_infos* client_infos)
{
    size_t              ret_send;
    size_t              ret_recv;
    udtcp_infos         save_client_infos;
    struct pollfd       poll_fd;
    int                 ret_poll;

    /* send udp port in client */
    ret_send = send(client_infos->tcp_socket,
        server->server_infos,
        sizeof(*(server->server_infos)), 0);
    if (ret_send != sizeof(*(server->server_infos)))
    {
        UDTCP_LOG_ERROR(server, "send: failed");
        return (-1);
    }

    /* poll wait receive */
    poll_fd.fd = client_infos->tcp_socket;
    poll_fd.events = POLLIN;
    ret_poll = poll(&poll_fd, 1, -1);
    if (ret_poll < 0)
    {
        UDTCP_LOG_ERROR(server, "poll: failed");
        return (-1);
    }
    else if (ret_poll == 0)
    {
        UDTCP_LOG_ERROR(server, "poll: timeout");
        return (-1);
    }

    /* save current client infos */
    memcpy(&save_client_infos, client_infos,
           sizeof(save_client_infos));

    /* receive to client udp port */
    ret_recv = recv(client_infos->tcp_socket,
        client_infos,
        sizeof(*client_infos) + 1, 0);
    /* socket is close */
    if (ret_recv == 0)
    {
        UDTCP_LOG_ERROR(server, "recv: close");
        return (-1);
    }
    /* bad size of return */
    if (ret_recv != sizeof(*client_infos))
    {
        UDTCP_LOG_ERROR(server, "recv: failed");
        return (-1);
    }

    /* applicate distant addr */
    client_infos->tcp_addr.sin_addr.s_addr =
        save_client_infos.tcp_addr.sin_addr.s_addr;
    client_infos->udp_server_addr.sin_addr.s_addr =
        save_client_infos.tcp_addr.sin_addr.s_addr;
    client_infos->udp_client_addr.sin_addr.s_addr =
        save_client_infos.tcp_addr.sin_addr.s_addr;

    /* applicate distant socket */
    client_infos->tcp_socket = save_client_infos.tcp_socket;
    client_infos->udp_server_socket = server->server_infos->udp_client_socket;
    client_infos->udp_client_socket = server->server_infos->udp_server_socket;
    return (0);
}

static int accept_client(udtcp_server* server)
{
    socklen_t       len_addr_client = sizeof(struct sockaddr_in);
    udtcp_infos*    new_client_infos;
    int             tmp_sock;

    /* check max connection */
    if (server->poll_nfds >= UDTCP_POLL_TABLE_SIZE)
    {
        tmp_sock = accept(server->server_infos->tcp_socket, NULL, NULL);
        if (tmp_sock == -1)
        {
            if (errno != EWOULDBLOCK)
            {
                UDTCP_LOG_ERROR(server, "accept: failed");
            }
            return (-1);
        }
        /* send '\0' for too many connect */
        send(tmp_sock, "", 1, 0);
        close(tmp_sock);
        UDTCP_LOG_ERROR(server, "accept: failed too many connection.");
        return (-1);
    }

    /* get address of new client infos */
    new_client_infos = &(server->infos[server->poll_nfds]);
    /* accept client */
    new_client_infos->tcp_socket =
        accept(server->server_infos->tcp_socket,
            (struct sockaddr*)&(new_client_infos->tcp_addr),
            &len_addr_client);
    if (new_client_infos->tcp_socket == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            UDTCP_LOG_ERROR(server, "accept: failed");
        }
        return (-1);
    }

    /* set no-block socket */
    if (udtcp_socket_add_option(new_client_infos->tcp_socket, O_NONBLOCK) == -1)
    {
        close(new_client_infos->tcp_socket);
        return (-1);
    }

    /* exchange informations */
    if (initialize_connection(server, new_client_infos) == -1)
    {
        close(new_client_infos->tcp_socket);
        UDTCP_LOG_ERROR(server, "accept: refused");
        return (-1);
    }

    /* assign send structure */
    new_client_infos->send = &(server->sends[server->poll_nfds]);

    /* copy infos */
    new_client_infos->id = server->count_id;
    udtcp_set_string_infos(new_client_infos,
        &(new_client_infos->tcp_addr));

    /* copy tcp port */
    new_client_infos->tcp_port =
        htons(new_client_infos->tcp_addr.sin_port);

    /* add in poll table */
    server->poll_fds[server->poll_nfds].fd = new_client_infos->tcp_socket;
    server->poll_fds[server->poll_nfds].events = POLLIN;

    UDTCP_LOG_INFO(server,
        "Connect ip: %s, "
        "tcp port: %u, "
        "udp server port: %u, "
        "udp client port: %u",
        new_client_infos->ip,
        (int)new_client_infos->tcp_port,
        (int)new_client_infos->udp_server_port,
        (int)new_client_infos->udp_client_port);

    /* use callback if defined */
    if (server->connect_callback != NULL)
        server->connect_callback(server, new_client_infos);

    ++server->poll_nfds;
    ++server->count_id;

    return (0);
}

static int receive_tcp(udtcp_server *server, udtcp_infos *client_infos)
{
    struct pollfd   poll_fd;
    ssize_t         ret_recv;
    uint32_t        buffer_size;
    uint32_t        data_size;
    int             ret_poll;
    uint8_t*        new_buffer_data;

    /* receive data size */
    ret_recv = recv(client_infos->tcp_socket,
        &data_size,
        sizeof(uint32_t), 0);
    /* recv error */
    if (ret_recv < 0)
        return (-1);
    /* recv close */
    if (ret_recv == 0)
        return (0);

    /* not a size of data */
    if (ret_recv != sizeof(uint32_t))
    {
        UDTCP_LOG_ERROR(server, "[%lu]%s:%hu TCP recv: bad size",
            client_infos->id,
            client_infos->hostname,
            client_infos->tcp_port);
        return (-1);
    }

    /* need reallocate buffer */
    if (data_size > server->buffer_size)
    {
        new_buffer_data = udtcp_new_buffer(data_size);
        /* malloc error */
        if (new_buffer_data == NULL)
        {
            UDTCP_LOG_ERROR(server, "malloc: %s", strerror(errno));
            return (-1);
        }
        /* delete last buffer */
        if (server->buffer_data != NULL)
            udtcp_free_buffer(server->buffer_data);
        /* set new buffer */
        server->buffer_data = new_buffer_data;
        server->buffer_size = data_size;
    }

    /* receive data */
    ret_recv = recv(client_infos->tcp_socket,
        server->buffer_data,
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
            poll_fd.fd = client_infos->tcp_socket;
            poll_fd.events = POLLIN;
            /* wait action from tcp socket */
            ret_poll = poll(&poll_fd, 1, -1);
            if (ret_poll < 0)
            {
                UDTCP_LOG_ERROR(server, "poll: failed");
                return (-1);
            }
            else if (ret_poll == 0)
            {
                UDTCP_LOG_ERROR(server, "poll: timeout");
                return (-1);
            }
            ret_recv = recv(client_infos->tcp_socket,
                server->buffer_data + buffer_size, data_size - buffer_size, 0);
            if (ret_recv < 0)
            {
                UDTCP_LOG_ERROR(server, "[%lu]%s:%hu TCP recv: bad size",
                    client_infos->id,
                    client_infos->hostname,
                    client_infos->tcp_port);
                return (-1);
            }
            if (ret_recv == 0)
                return (0);
            buffer_size += ret_recv;
        }
    }

    /* use callback if defined */
    if (server->receive_tcp_callback != NULL)
    {
        server->receive_tcp_callback(server,
            client_infos, server->buffer_data, data_size);
    }

    return (data_size);
}

static int udtcp_get_infos(udtcp_server *server,
    struct sockaddr_in* in_addr, udtcp_infos** out_infos)
{
    size_t i;

    /* search in clients */
    for (i = 2 ; i < server->poll_nfds ; ++i)
    {
        if (server->infos[i].udp_client_addr.sin_addr.s_addr == in_addr->sin_addr.s_addr
         && server->infos[i].udp_client_addr.sin_port == in_addr->sin_port)
        {
            *out_infos = &(server->infos[i]);
            return (0);
        }
    }
    *out_infos = NULL;
    return (-1);
}

static int receive_udp(udtcp_server *server, udtcp_infos *server_infos)
{
    socklen_t           len_addr = sizeof(struct sockaddr_in);
    udtcp_infos*        client_infos;
    uint8_t*            new_buffer_data;
    uint32_t            data_size;
    int                 ret_recv;
    struct sockaddr_in  addr;

    /* receive data size */
    ret_recv = recvfrom(server_infos->udp_server_socket,
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
        UDTCP_LOG_ERROR(server, "[%lu]%s:%hu UDP recv: bad size (%d)",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port, ret_recv);
        return (-1);
    }

    /* need reallocate buffer */
    if (sizeof(uint32_t) + data_size > server->buffer_size)
    {
        new_buffer_data = udtcp_new_buffer(sizeof(uint32_t) + data_size);
        /* malloc error */
        if (new_buffer_data == NULL)
        {
            UDTCP_LOG_ERROR(server, "[%lu]%s:%hu TCP recv: size too big (%u)",
                server_infos->id,
                server_infos->hostname,
                server_infos->udp_server_port, data_size);
            return (-1);
        }
        /* delete last buffer */
        if (server->buffer_data != NULL)
            udtcp_free_buffer(server->buffer_data);
        /* set new buffer */
        server->buffer_data = new_buffer_data;
        server->buffer_size = data_size + sizeof(uint32_t);
    }

    len_addr = sizeof(struct sockaddr_in);
    ret_recv = recvfrom(server_infos->udp_server_socket,
        server->buffer_data, sizeof(uint32_t) + data_size, 0,
        (struct sockaddr*)&addr,
        &len_addr);
    if (ret_recv < 0)
    {
        UDTCP_LOG_ERROR(server, "[%lu]%s:%hu UDP recv: fail",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }
    if (ret_recv == 0)
        return (0);
    if (ret_recv - sizeof(uint32_t) != data_size)
    {
        UDTCP_LOG_ERROR(server, "[%lu]%s:%hu UDP recv: bad size",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }

    if (udtcp_get_infos(server, &addr, &client_infos) == -1)
    {
        UDTCP_LOG_ERROR(server, "[%lu]%s:%hu UDP udtcp_get_infos: bad client",
            server_infos->id,
            server_infos->hostname,
            server_infos->udp_server_port);
        return (-1);
    }

    /* use callback if defined */
    if (server->receive_udp_callback != NULL)
        server->receive_udp_callback(server, client_infos, server->buffer_data + sizeof(uint32_t), data_size);

    return (data_size);
}

static void disconnect(udtcp_server *server, udtcp_infos *client_infos)
{
    /* send '\0' to the client */
    if (shutdown(client_infos->tcp_socket, SHUT_RDWR) == -1)
    {
        UDTCP_LOG_ERROR(server, "shutdown: fail");
    }
    /* close tcp socket */
    if (close(client_infos->tcp_socket) == -1)
    {
        UDTCP_LOG_ERROR(server, "close: fail");
    }
    UDTCP_LOG_INFO(server,
        "Disconnect ip: %s, "
        "tcp port: %u, "
        "udp server port: %u, "
        "udp client port: %u",
        client_infos->ip,
        (unsigned int)client_infos->tcp_port,
        (unsigned int)client_infos->udp_server_port,
        (unsigned int)client_infos->udp_client_port);
    /* use callback if defined */
    if (server->disconnect_callback != NULL)
        server->disconnect_callback(server, client_infos);
}

__attribute__((weak))
enum udtcp_poll_e udtcp_server_poll(udtcp_server *server, long timeout)
{
    size_t i;
    int ret_poll;
    int ret_receive;
    int organise_array = 0;

    /* wait action from sockets */
    ret_poll = poll(server->poll_fds, server->poll_nfds, timeout);
    /* error or receive signal */
    if (ret_poll < 0)
    {
        if (errno == EINTR)
            return (UDTCP_POLL_SIGNAL);
        UDTCP_LOG_ERROR(server, "poll: fail \"%s\"", strerror(errno));
        return (UDTCP_POLL_ERROR);
    }
    /* timeout */
    if (ret_poll == 0)
    {
        UDTCP_LOG_INFO(server, "poll: timeout");
        return (UDTCP_POLL_TIMEOUT);
    }

    /* foreach socket */
    for (i = 0; server->poll_loop && i < server->poll_nfds; ++i)
    {
        /* not active fd */
        if (!server->poll_fds[i].revents)
            continue;

        if (!(server->poll_fds[i].revents & POLLIN))
        {
            UDTCP_LOG_ERROR(server, "poll: revent failed");
            continue;
        }

        /* is tcp server */
        if (server->poll_fds[i].fd == server->server_infos->tcp_socket)
        {
            ret_receive = accept_client(server);
            while (server->poll_loop && ret_receive == 0)
                ret_receive = accept_client(server);
        }
        /* is udp server */
        else if (server->poll_fds[i].fd == server->server_infos->udp_server_socket)
        {
            ret_receive = receive_udp(server, server->server_infos);
            while (server->poll_loop && ret_receive > 0)
                ret_receive = receive_udp(server, server->server_infos);
        }
        /* is client */
        else
        {
            ret_receive = receive_tcp(server, &(server->infos[i]));
            while (server->poll_loop && ret_receive > 0)
                ret_receive = receive_tcp(server, &(server->infos[i]));
            /* error socket */
            if (ret_receive < 0)
            {
                /* not nonblock errno */
                if (errno != EWOULDBLOCK)
                {
                    disconnect(server, &(server->infos[i]));
                    server->poll_fds[i].fd = -1;
                    organise_array = 1;
                }
            }
            /* close socket */
            else if (ret_receive == 0)
            {
                disconnect(server, &(server->infos[i]));
                server->poll_fds[i].fd = -1;
                organise_array = 1;
            }
        }
    }
    if (organise_array)
    {
        /* compact poll table */
        for (i = 2; i < server->poll_nfds; ++i)
        {
            if (server->poll_fds[i].fd == -1)
            {
                if (i < server->poll_nfds - 1)
                {
                    memcpy(&(server->poll_fds[i]),
                        &(server->poll_fds[i + 1]),
                        sizeof(struct pollfd) * (server->poll_nfds - i));
                    memcpy(&(server->infos[i]),
                        &(server->infos[i + 1]),
                        sizeof(udtcp_infos) * (server->poll_nfds - i));
                }
                --i;
                --server->poll_nfds;
            }
        }
    }
    return (UDTCP_POLL_SUCCESS);
}
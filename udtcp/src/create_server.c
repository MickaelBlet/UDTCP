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

#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "udtcp.h"
#include "udtcp_utils.h"

__attribute__((weak))
int udtcp_create_server(const char* hostname,
    uint16_t tcp_port, uint16_t udp_server_port, uint16_t udp_client_port,
    udtcp_server** out_server)
{
    socklen_t           len_addr = sizeof(struct sockaddr_in);
    udtcp_server*       server;
    struct hostent*     host_entity;
    in_addr_t           host_addr;
    size_t              i;

    /* get host list by name */
    host_entity = gethostbyname(hostname);
    if (host_entity == NULL)
    {
        errno = EHOSTUNREACH;
        return (-1);
    }

    /* copy the first address in hostname list */
    host_addr = *((in_addr_t*)host_entity->h_addr_list[0]);

    /* create a new server */
    server = udtcp_new_server();
    if (server == NULL)
        return (-1);

    /* clean memory */
    memset(server, 0, sizeof(udtcp_server));

    /* initialize infos */
    for (i = 2; i < UDTCP_POLL_TABLE_SIZE; ++i)
    {
        /* init send mutex */
        if (pthread_mutex_init(&(server->sends[i].mutex), NULL) != 0)
        {
            for (i = i - 1; i > 2; --i)
                pthread_mutex_destroy(&(server->sends[i].mutex));
            udtcp_free_server(server);
            return (-1);
        }
    }

    /* server is the first element in infos */
    server->server_infos = &(server->infos[0]);
    server->clients_infos = &(server->infos[2]);

    /* create tcp socket */
    server->server_infos->tcp_socket = socket(AF_INET,
                                                  SOCK_STREAM,
                                                  IPPROTO_TCP);
    if (server->server_infos->tcp_socket < 0)
    {
        for (i = 2; i < UDTCP_POLL_TABLE_SIZE; ++i)
            pthread_mutex_destroy(&(server->sends[i].mutex));
        udtcp_free_server(server);
        return (-1);
    }
    /* define poll position */
    server->poll_fds[0].fd = server->server_infos->tcp_socket;
    server->poll_fds[0].events = POLLIN;
    server->poll_nfds = 1;

    /* create udp server socket */
    server->server_infos->udp_server_socket = socket(AF_INET,
                                                         SOCK_DGRAM,
                                                         IPPROTO_UDP);
    if (server->server_infos->udp_server_socket < 0)
    {
        for (i = 2; i < UDTCP_POLL_TABLE_SIZE; ++i)
            pthread_mutex_destroy(&(server->sends[i].mutex));
        close(server->server_infos->tcp_socket);
        udtcp_free_server(server);
        return (-1);
    }
    /* define poll position */
    server->poll_fds[1].fd = server->server_infos->udp_server_socket;
    server->poll_fds[1].events = POLLIN;
    server->poll_nfds = 2;

    /* create udp client socket */
    server->server_infos->udp_client_socket = socket(AF_INET,
                                                         SOCK_DGRAM,
                                                         IPPROTO_UDP);
    if (server->server_infos->udp_client_socket < 0)
    {
        for (i = 2; i < UDTCP_POLL_TABLE_SIZE; ++i)
            pthread_mutex_destroy(&(server->sends[i].mutex));
        close(server->server_infos->tcp_socket);
        close(server->server_infos->udp_server_socket);
        udtcp_free_server(server);
        return (-1);
    }

    /* bind tcp socket */
    server->server_infos->tcp_addr.sin_family =
        AF_INET;
    server->server_infos->tcp_addr.sin_addr.s_addr =
        htonl(host_addr);
    server->server_infos->tcp_addr.sin_port =
        htons(tcp_port);

    /* bind tcp port into socket */
    if (bind(server->server_infos->tcp_socket,
        (struct sockaddr*)&(server->server_infos->tcp_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_server(server);
        return (-1);
    }
    if (getsockname(server->server_infos->tcp_socket,
        (struct sockaddr*)&(server->server_infos->tcp_addr),
        &len_addr) == -1)
    {
        udtcp_delete_server(server);
        return (-1);
    }

    /* bind udp server socket */
    server->server_infos->udp_server_addr.sin_family =
        AF_INET;
    server->server_infos->udp_server_addr.sin_addr.s_addr =
        htonl(host_addr);
    server->server_infos->udp_server_addr.sin_port =
        htons(udp_server_port);

    /* bind udp port into socket */
    if (bind(server->server_infos->udp_server_socket,
        (struct sockaddr*)&(server->server_infos->udp_server_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_server(server);
        return (-1);
    }
    if (getsockname(server->server_infos->udp_server_socket,
        (struct sockaddr*)&(server->server_infos->udp_server_addr),
        &len_addr) == -1)
    {
        udtcp_delete_server(server);
        return (-1);
    }

    /* bind udp client socket */
    server->server_infos->udp_client_addr.sin_family =
        AF_INET;
    server->server_infos->udp_client_addr.sin_addr.s_addr =
        htonl(host_addr);
    server->server_infos->udp_client_addr.sin_port =
        htons(udp_client_port);

    /* bind udp port into socket */
    if (bind(server->server_infos->udp_client_socket,
        (struct sockaddr*)&(server->server_infos->udp_client_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_server(server);
        return (-1);
    }
    if (getsockname(server->server_infos->udp_client_socket,
        (struct sockaddr*)&(server->server_infos->udp_client_addr),
        &len_addr) == -1)
    {
        udtcp_delete_server(server);
        return (-1);
    }

    /* get port from addr */
    server->server_infos->tcp_port =
        htons(server->server_infos->tcp_addr.sin_port);
    server->server_infos->udp_server_port =
        htons(server->server_infos->udp_server_addr.sin_port);
    server->server_infos->udp_client_port =
        htons(server->server_infos->udp_client_addr.sin_port);

    /* prepare to accept client */
    if (listen(server->server_infos->tcp_socket, 10) != 0)
    {
        udtcp_delete_server(server);
        return (-1);
    }

    /* set no block socket */
    if (udtcp_socket_add_option(server->server_infos->tcp_socket,
        O_NONBLOCK) == -1)
    {
        udtcp_delete_server(server);
        return (-1);
    }
    if (udtcp_socket_add_option(server->server_infos->udp_server_socket,
        O_NONBLOCK) == -1)
    {
        udtcp_delete_server(server);
        return (-1);
    }

    /* copy infos */
    server->server_infos->id = 0;
    udtcp_set_string_infos(server->server_infos,
        &(server->server_infos->tcp_addr));

    /* start counter */
    server->count_id = 1;

    *out_server = server;

    return (0);
}
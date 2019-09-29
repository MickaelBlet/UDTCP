#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "udtcp.h"

int udtcp_create_server(const char* hostname,
    uint16_t tcp_port, uint16_t udp_server_port, uint16_t udp_client_port,
    udtcp_server** out_server)
{
    socklen_t           len_addr;
    udtcp_server*       new_server;
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
    new_server = (udtcp_server*)malloc(sizeof(udtcp_server));
    if (new_server == NULL)
        return (-1);

    /* clean memory */
    memset(new_server, 0, sizeof(udtcp_server));

    /* initialize infos */
    for (i = 2; i < UDTCP_POLL_TABLE_SIZE; ++i)
    {
        /* init send mutex */
        if (pthread_mutex_init(&(new_server->sends[i].mutex), NULL) != 0)
        {
            for (i = i - 1; i > 2; --i)
                pthread_mutex_destroy(&(new_server->sends[i].mutex));
            free(new_server);
            return (-1);
        }
    }

    /* server is the first element in infos */
    new_server->server_infos = &(new_server->infos[0]);
    new_server->clients_infos = &(new_server->infos[2]);

    /* create tcp socket */
    new_server->server_infos->tcp_socket = socket(AF_INET,
                                                  SOCK_STREAM,
                                                  IPPROTO_TCP);
    if (new_server->server_infos->tcp_socket < 0)
    {
        free(new_server);
        return (-1);
    }
    /* define poll position */
    new_server->poll_fds[0].fd = new_server->server_infos->tcp_socket;
    new_server->poll_fds[0].events = POLLIN;
    new_server->poll_nfds = 1;

    /* create udp server socket */
    new_server->server_infos->udp_server_socket = socket(AF_INET,
                                                         SOCK_DGRAM,
                                                         IPPROTO_UDP);
    if (new_server->server_infos->udp_server_socket < 0)
    {
        close(new_server->server_infos->tcp_socket);
        free(new_server);
        return (-1);
    }
    /* define poll position */
    new_server->poll_fds[1].fd = new_server->server_infos->udp_server_socket;
    new_server->poll_fds[1].events = POLLIN;
    new_server->poll_nfds = 2;

    /* create udp client socket */
    new_server->server_infos->udp_client_socket = socket(AF_INET,
                                                         SOCK_DGRAM,
                                                         IPPROTO_UDP);
    if (new_server->server_infos->udp_client_socket < 0)
    {
        close(new_server->server_infos->tcp_socket);
        close(new_server->server_infos->udp_server_socket);
        free(new_server);
        return (-1);
    }

    /* bind tcp socket */
    new_server->server_infos->tcp_addr.sin_family = AF_INET;
    new_server->server_infos->tcp_addr.sin_addr.s_addr = htonl(host_addr);
    new_server->server_infos->tcp_addr.sin_port = htons(tcp_port);

    /* bind tcp port into socket */
    if (bind(new_server->server_infos->tcp_socket,
        (struct sockaddr*)&(new_server->server_infos->tcp_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }
    if (getsockname(new_server->server_infos->tcp_socket,
        (struct sockaddr*)&(new_server->server_infos->tcp_addr),
        &len_addr) == -1)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }

    /* bind udp server socket */
    new_server->server_infos->udp_server_addr.sin_family = AF_INET;
    new_server->server_infos->udp_server_addr.sin_addr.s_addr = htonl(host_addr);
    new_server->server_infos->udp_server_addr.sin_port = htons(udp_server_port);

    /* bind udp port into socket */
    if (bind(new_server->server_infos->udp_server_socket,
        (struct sockaddr*)&(new_server->server_infos->udp_server_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }
    if (getsockname(new_server->server_infos->udp_server_socket,
        (struct sockaddr*)&(new_server->server_infos->udp_server_addr),
        &len_addr) == -1)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }

    /* bind udp client socket */
    new_server->server_infos->udp_client_addr.sin_family = AF_INET;
    new_server->server_infos->udp_client_addr.sin_addr.s_addr = htonl(host_addr);
    new_server->server_infos->udp_client_addr.sin_port = htons(udp_client_port);

    /* bind udp port into socket */
    if (bind(new_server->server_infos->udp_client_socket,
        (struct sockaddr*)&(new_server->server_infos->udp_client_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }
    if (getsockname(new_server->server_infos->udp_client_socket,
        (struct sockaddr*)&(new_server->server_infos->udp_client_addr),
        &len_addr) == -1)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }

    /* get port from addr */
    new_server->server_infos->tcp_port =
        htons(new_server->server_infos->tcp_addr.sin_port);
    new_server->server_infos->udp_server_port =
        htons(new_server->server_infos->udp_server_addr.sin_port);
    new_server->server_infos->udp_client_port =
        htons(new_server->server_infos->udp_client_addr.sin_port);

    /* prepare to accept client */
    if (listen(new_server->server_infos->tcp_socket, 10) != 0)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }

    /* set no block socket */
    if (udtcp_socket_add_option(new_server->server_infos->tcp_socket,
        O_NONBLOCK) == -1)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }
    if (udtcp_socket_add_option(new_server->server_infos->udp_server_socket,
        O_NONBLOCK) == -1)
    {
        udtcp_delete_server(&new_server);
        return (-1);
    }

    /* copy infos */
    new_server->server_infos->id = 0;
    udtcp_set_string_infos(new_server->server_infos,
        &(new_server->server_infos->tcp_addr));

    /* start counter */
    new_server->count_id = 1;

    *out_server = new_server;

    return (0);
}
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "udtcp.h"

int udtcp_create_client(const char* hostname,
    uint16_t tcp_port, uint16_t udp_server_port, uint16_t udp_client_port,
    udtcp_client** out_client)
{
    socklen_t           len_addr = sizeof(struct sockaddr_in);
    udtcp_client*       new_client;
    struct hostent*     host_entity;
    in_addr_t           host_addr;
    const char*         ip;

    /* get host list by name */
    host_entity = gethostbyname(hostname);
    if (host_entity == NULL)
    {
        errno = EHOSTUNREACH;
        return (-1);
    }

    /* copy the first address in hostname list */
    host_addr = *((in_addr_t*)host_entity->h_addr_list[0]);

    /* create a new client */
    new_client = (udtcp_client*)malloc(sizeof(udtcp_client));
    if (new_client == NULL)
        return (-1);

    /* clean memory */
    memset(new_client, 0, sizeof(udtcp_client));

    /* init send mutex */
    if (pthread_mutex_init(&(new_client->send.mutex), NULL) != 0)
    {
        udtcp_delete_client(&new_client);
        return (-1);
    }

    /* set shorcut infos pointer */
    new_client->client_infos = &(new_client->infos[0]);
    new_client->server_infos = &(new_client->infos[1]);

    /* create tcp socket */
    new_client->client_infos->tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_client->client_infos->tcp_socket < 0)
    {
        free(new_client);
        return (-1);
    }
    /* create udp server socket */
    new_client->client_infos->udp_server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (new_client->client_infos->udp_server_socket < 0)
    {
        close(new_client->client_infos->tcp_socket);
        free(new_client);
        return (-1);
    }
    /* create udp client socket */
    new_client->client_infos->udp_client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (new_client->client_infos->udp_client_socket < 0)
    {
        close(new_client->client_infos->tcp_socket);
        close(new_client->client_infos->udp_server_socket);
        free(new_client);
        return (-1);
    }

    /* active re use addr in socket */
    int yes = 1;
    if (setsockopt(new_client->client_infos->tcp_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1
     || setsockopt(new_client->client_infos->udp_server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1
     || setsockopt(new_client->client_infos->udp_client_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        udtcp_delete_client(&new_client);
        return (-1);
    }

#ifdef SO_REUSEPORT
    if (setsockopt(new_client->client_infos->tcp_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1
     || setsockopt(new_client->client_infos->udp_server_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1
     || setsockopt(new_client->client_infos->udp_server_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1)
    {
        udtcp_delete_client(&new_client);
        return (-1);
    }
#endif

    /* bind tcp socket */
    new_client->client_infos->tcp_addr.sin_family = AF_INET;
    new_client->client_infos->tcp_addr.sin_addr.s_addr = host_addr;
    new_client->client_infos->tcp_addr.sin_port = htons(tcp_port);

    /* bind tcp port into socket */
    if (bind(new_client->client_infos->tcp_socket, (struct sockaddr*)&(new_client->client_infos->tcp_addr), sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }
    if (getsockname(new_client->client_infos->tcp_socket, (struct sockaddr*)&(new_client->client_infos->tcp_addr), &len_addr) == -1)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }

    /* bind udp server socket */
    new_client->client_infos->udp_server_addr.sin_family = AF_INET;
    new_client->client_infos->udp_server_addr.sin_addr.s_addr = host_addr;
    new_client->client_infos->udp_server_addr.sin_port = htons(udp_server_port);

    /* bind udp port into socket */
    if (bind(new_client->client_infos->udp_server_socket, (struct sockaddr*)&(new_client->client_infos->udp_server_addr), sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }
    if (getsockname(new_client->client_infos->udp_server_socket, (struct sockaddr*)&(new_client->client_infos->udp_server_addr), &len_addr) == -1)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }

    /* bind udp client socket */
    new_client->client_infos->udp_client_addr.sin_family = AF_INET;
    new_client->client_infos->udp_client_addr.sin_addr.s_addr = host_addr;
    new_client->client_infos->udp_client_addr.sin_port = htons(udp_client_port);

    /* bind udp port into socket */
    if (bind(new_client->client_infos->udp_client_socket, (struct sockaddr*)&(new_client->client_infos->udp_client_addr), sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }
    if (getsockname(new_client->client_infos->udp_client_socket, (struct sockaddr*)&(new_client->client_infos->udp_client_addr), &len_addr) == -1)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }

    /* get port from addr */
    new_client->client_infos->tcp_port = htons(new_client->client_infos->tcp_addr.sin_port);
    new_client->client_infos->udp_server_port = htons(new_client->client_infos->udp_server_addr.sin_port);
    new_client->client_infos->udp_client_port = htons(new_client->client_infos->udp_client_addr.sin_port);

    /* set no block socket */
    if (udtcp_socket_add_option(new_client->client_infos->tcp_socket, O_NONBLOCK) == -1)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }
    if (udtcp_socket_add_option(new_client->client_infos->udp_server_socket, O_NONBLOCK) == -1)
    {
        udtcp_delete_client(&new_client);
        return -1;
    }

    /* define poll position */
    new_client->poll_fds[0].fd = new_client->client_infos->tcp_socket;
    new_client->poll_fds[0].events = POLLIN;
    new_client->poll_fds[1].fd = new_client->client_infos->udp_server_socket;
    new_client->poll_fds[1].events = POLLIN;
    new_client->poll_nfds = 2;

    /* copy infos */
    new_client->client_infos->id = 0;
    memcpy(new_client->client_infos->hostname, host_entity->h_name, strlen(host_entity->h_name));
    ip = inet_ntoa(new_client->client_infos->tcp_addr.sin_addr);
    memcpy(new_client->client_infos->ip, ip, strlen(ip));

    /* define default callback */
    new_client->connect_callback = NULL;
    new_client->disconnect_callback = NULL;
    new_client->receive_tcp_callback = NULL;
    new_client->receive_udp_callback = NULL;
    new_client->log_callback = NULL;

    /* initialize poll loop */
    new_client->poll_loop = 0;

    /* initialize buffer */
    new_client->buffer_data = NULL;
    new_client->buffer_size = 0;

    /* it's recreate at connection */
    close(new_client->client_infos->tcp_socket);

    *out_client = new_client;

    return 0;
}
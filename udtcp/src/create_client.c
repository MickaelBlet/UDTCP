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
    udtcp_client*       client;
    socklen_t           len_addr = sizeof(struct sockaddr_in);
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
    memcpy(&host_addr, host_entity->h_addr_list[0], sizeof(in_addr_t));

    /* create a new client */
    client = (udtcp_client*)malloc(sizeof(udtcp_client));
    if (client == NULL)
        return (-1);

    /* clean memory */
    memset(client, 0, sizeof(udtcp_client));

    /* init send mutex */
    if (pthread_mutex_init(&(client->send.mutex), NULL) != 0)
    {
        udtcp_delete_client(&client);
        return (-1);
    }

    /* set shorcut infos pointer */
    client->client_infos = &(client->infos[0]);
    client->server_infos = &(client->infos[1]);

    /* create tcp socket */
    client->client_infos->tcp_socket = socket(AF_INET,
                                                  SOCK_STREAM,
                                                  IPPROTO_TCP);
    if (client->client_infos->tcp_socket < 0)
    {
        free(client);
        return (-1);
    }
    /* create udp server socket */
    client->client_infos->udp_server_socket = socket(AF_INET,
                                                         SOCK_DGRAM,
                                                         IPPROTO_UDP);
    if (client->client_infos->udp_server_socket < 0)
    {
        close(client->client_infos->tcp_socket);
        free(client);
        return (-1);
    }
    /* create udp client socket */
    client->client_infos->udp_client_socket = socket(AF_INET,
                                                         SOCK_DGRAM,
                                                         IPPROTO_UDP);
    if (client->client_infos->udp_client_socket < 0)
    {
        close(client->client_infos->tcp_socket);
        close(client->client_infos->udp_server_socket);
        free(client);
        return (-1);
    }

    /* active re use addr in socket */
    int yes = 1;
    if (setsockopt(client->client_infos->tcp_socket,
                   SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1
     || setsockopt(client->client_infos->udp_server_socket,
                   SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1
     || setsockopt(client->client_infos->udp_client_socket,
                   SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        udtcp_delete_client(&client);
        return (-1);
    }

#ifdef SO_REUSEPORT
    if (setsockopt(client->client_infos->tcp_socket,
                   SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1
     || setsockopt(client->client_infos->udp_server_socket,
                   SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1
     || setsockopt(client->client_infos->udp_server_socket,
                   SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1)
    {
        udtcp_delete_client(&client);
        return (-1);
    }
#endif

    /* bind tcp socket */
    client->client_infos->tcp_addr.sin_family =
        AF_INET;
    client->client_infos->tcp_addr.sin_addr.s_addr =
        htonl(host_addr);
    client->client_infos->tcp_addr.sin_port =
        htons(tcp_port);

    /* bind tcp port into socket */
    if (bind(client->client_infos->tcp_socket,
        (struct sockaddr*)&(client->client_infos->tcp_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_client(&client);
        return -1;
    }
    if (getsockname(client->client_infos->tcp_socket,
        (struct sockaddr*)&(client->client_infos->tcp_addr),
        &len_addr) == -1)
    {
        udtcp_delete_client(&client);
        return -1;
    }

    /* bind udp server socket */
    client->client_infos->udp_server_addr.sin_family =
        AF_INET;
    client->client_infos->udp_server_addr.sin_addr.s_addr =
        htonl(host_addr);
    client->client_infos->udp_server_addr.sin_port =
        htons(udp_server_port);

    /* bind udp port into socket */
    if (bind(client->client_infos->udp_server_socket,
        (struct sockaddr*)&(client->client_infos->udp_server_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_client(&client);
        return -1;
    }
    if (getsockname(client->client_infos->udp_server_socket,
        (struct sockaddr*)&(client->client_infos->udp_server_addr),
        &len_addr) == -1)
    {
        udtcp_delete_client(&client);
        return -1;
    }

    /* bind udp client socket */
    client->client_infos->udp_client_addr.sin_family =
        AF_INET;
    client->client_infos->udp_client_addr.sin_addr.s_addr =
        htonl(host_addr);
    client->client_infos->udp_client_addr.sin_port =
        htons(udp_client_port);

    /* bind udp port into socket */
    if (bind(client->client_infos->udp_client_socket,
        (struct sockaddr*)&(client->client_infos->udp_client_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        udtcp_delete_client(&client);
        return -1;
    }
    if (getsockname(client->client_infos->udp_client_socket,
        (struct sockaddr*)&(client->client_infos->udp_client_addr),
        &len_addr) == -1)
    {
        udtcp_delete_client(&client);
        return -1;
    }

    /* get port from addr */
    client->client_infos->tcp_port =
        htons(client->client_infos->tcp_addr.sin_port);
    client->client_infos->udp_server_port =
        htons(client->client_infos->udp_server_addr.sin_port);
    client->client_infos->udp_client_port =
        htons(client->client_infos->udp_client_addr.sin_port);

    /* set no block socket */
    if (udtcp_socket_add_option(client->client_infos->tcp_socket,
        O_NONBLOCK) == -1)
    {
        udtcp_delete_client(&client);
        return -1;
    }
    if (udtcp_socket_add_option(client->client_infos->udp_server_socket,
        O_NONBLOCK) == -1)
    {
        udtcp_delete_client(&client);
        return -1;
    }

    /* define poll position */
    client->poll_fds[0].fd = client->client_infos->tcp_socket;
    client->poll_fds[0].events = POLLIN;
    client->poll_fds[1].fd = client->client_infos->udp_server_socket;
    client->poll_fds[1].events = POLLIN;
    client->poll_nfds = 2;

    /* copy infos */
    client->client_infos->id = 0;
    memcpy(client->client_infos->hostname,
           host_entity->h_name, strlen(host_entity->h_name));
    ip = inet_ntoa(client->client_infos->tcp_addr.sin_addr);
    memcpy(client->client_infos->ip, ip, strlen(ip));

    /* define default callback */
    client->connect_callback = NULL;
    client->disconnect_callback = NULL;
    client->receive_tcp_callback = NULL;
    client->receive_udp_callback = NULL;
    client->log_callback = NULL;

    /* initialize poll loop */
    client->poll_loop = 0;

    /* initialize buffer */
    client->buffer_data = NULL;
    client->buffer_size = 0;

    /* it's recreate at connection */
    close(client->client_infos->tcp_socket);

    *out_client = client;

    return 0;
}
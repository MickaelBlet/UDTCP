#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "udtcp.h"

static void close_tcp_socket(udtcp_client* client)
{
    /* send '\0' to the server */
    shutdown(client->client_infos->tcp_socket, SHUT_RDWR);
    /* close old socket */
    close(client->client_infos->tcp_socket);
}

static int create_tcp_socket(udtcp_client* client)
{
    /* create tcp socket */
    client->client_infos->tcp_socket = socket(AF_INET,
                                              SOCK_STREAM, IPPROTO_TCP);
    if (client->client_infos->tcp_socket == -1)
    {
        UDTCP_LOG_ERROR(client, "socket: %s", strerror(errno));
        return (-1);
    }
    /* active re use addr in socket */
    int yes = 1;
    if (setsockopt(client->client_infos->tcp_socket,
        SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        UDTCP_LOG_ERROR(client, "setsockopt: %s", strerror(errno));
        return (-1);
    }
#ifdef SO_REUSEPORT
    if (setsockopt(client->client_infos->tcp_socket,
        SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1)
    {
        UDTCP_LOG_ERROR(client, "setsockopt: %s", strerror(errno));
        return (-1);
    }
#endif
    /* add non block option */
    if (udtcp_socket_add_option(client->client_infos->tcp_socket,
        O_NONBLOCK) == -1)
    {
        UDTCP_LOG_ERROR(client, "fcntl: %s", strerror(errno));
        return (-1);
    }
    /* bind tcp port into socket */
    if (bind(client->client_infos->tcp_socket,
        (struct sockaddr*)&(client->client_infos->tcp_addr),
        sizeof(struct sockaddr_in)) != 0)
    {
        UDTCP_LOG_ERROR(client, "setsockopt: %s", strerror(errno));
        return (-1);
    }
    return (0);
}

static int initialize_connection(udtcp_client* client)
{
    struct pollfd       poll_fd;
    int                 ret_poll;
    ssize_t             ret_send;
    ssize_t             ret_recv;

    /* prepare poll */
    poll_fd.fd = client->client_infos->tcp_socket;
    poll_fd.events = POLLIN;
    /* try wait receive */
    ret_poll = poll(&poll_fd, 1, -1);
    if (ret_poll < 0)
    {
        UDTCP_LOG_ERROR(client, "poll: fail");
        return (UDTCP_CONNECT_ERROR);
    }
    /* poll timeout */
    else if (ret_poll == 0)
    {
        UDTCP_LOG_ERROR(client, "poll: timeout");
        errno = ETIMEDOUT;
        return (UDTCP_CONNECT_ERROR);
    }

    /* receive addr from server */
    ret_recv = recv(client->client_infos->tcp_socket,
        client->server_infos,
        sizeof(udtcp_infos) + 1, 0);
    /* socket is close */
    if (ret_recv == 0)
    {
        UDTCP_LOG_ERROR(client, "recv: close");
        return (UDTCP_CONNECT_ERROR);
    }
    /* bad size of return */
    if (ret_recv != sizeof(udtcp_infos))
    {
        if (ret_recv != 1)
        {
            UDTCP_LOG_ERROR(client, "recv: fail");
            return (UDTCP_CONNECT_ERROR);
        }
        else
        {
            UDTCP_LOG_ERROR(client, "connect: too many connection");
            return (UDTCP_CONNECT_TOO_MANY);
        }
    }

    /* send to server udp port */
    ret_send = send(client->client_infos->tcp_socket,
        client->client_infos,
        sizeof(udtcp_infos), 0);
    /* socket is close */
    if (ret_recv == 0)
    {
        UDTCP_LOG_ERROR(client, "send: close");
        return (UDTCP_CONNECT_ERROR);
    }
    /* bad size of return */
    if (ret_send != sizeof(udtcp_infos))
    {
        UDTCP_LOG_ERROR(client, "recv: fail");
        return (UDTCP_CONNECT_ERROR);
    }
    return (UDTCP_CONNECT_SUCCESS);
}

enum udtcp_connect_e udtcp_connect_client(udtcp_client* client,
    const char* hostname, uint16_t tcp_port, long timeout)
{
    struct hostent*         host_entity;
    in_addr_t               host_addr;
    struct pollfd           poll_fd;
    int                     ret_poll;
    enum udtcp_connect_e    ret_initialize_connection;

    /* create_tcp_socket */
    if (create_tcp_socket(client) == -1)
    {
        close_tcp_socket(client);
        return (UDTCP_CONNECT_ERROR);
    }
    /* get host list by name */
    host_entity = gethostbyname(hostname);
    if (host_entity == NULL)
    {
        UDTCP_LOG_ERROR(client, "gethostbyname: fail");
        return (UDTCP_CONNECT_ERROR);
    }

    /* copy the first address in hostname list */
    host_addr = *((in_addr_t*)(host_entity->h_addr_list[0]));

    /* clean memory */
    memset(&(client->server_infos->tcp_addr), 0, sizeof(struct sockaddr_in));

    /* create tcp server addr */
    client->server_infos->tcp_addr.sin_family = AF_INET;
    client->server_infos->tcp_addr.sin_addr.s_addr = host_addr;
    client->server_infos->tcp_addr.sin_port = htons(tcp_port);

    /* try to connect */
    if (connect(client->client_infos->tcp_socket,
        (struct sockaddr*)&(client->server_infos->tcp_addr),
        sizeof(struct sockaddr_in)) < 0)
    {
        /* wait connect */
        if (errno == EINPROGRESS)
        {
            /* prepare poll */
            poll_fd.fd = client->client_infos->tcp_socket;
            poll_fd.events = POLLERR | POLLHUP | POLLIN | POLLOUT;
            /* try wait connect */
            ret_poll = poll(&poll_fd, 1, timeout);
            if (ret_poll < 0)
            {
                close_tcp_socket(client);
                UDTCP_LOG_ERROR(client, "poll: fail \"%s\"",
                    strerror(errno));
                return (UDTCP_CONNECT_ERROR);
            }
            /* poll timeout */
            else if (ret_poll == 0)
            {
                close_tcp_socket(client);
                errno = ETIMEDOUT;
                UDTCP_LOG_INFO(client, "poll: timeout \"%s\"", strerror(errno));
                return (UDTCP_CONNECT_TIMEOUT);
            }
            /* still not connect */
            else if (poll_fd.revents & POLLHUP)
            {
                close_tcp_socket(client);
                errno = ECONNREFUSED;
                UDTCP_LOG_ERROR(client, "connect: fail \"%s\"",
                    strerror(errno));
                return (UDTCP_CONNECT_ERROR);
            }
        }
        /* bad error */
        else
        {
            close_tcp_socket(client);
            UDTCP_LOG_ERROR(client, "connect: fail \"%s\"", strerror(errno));
            return (UDTCP_CONNECT_ERROR);
        }
    }

    /* initialize the connection */
    ret_initialize_connection = initialize_connection(client);
    if (ret_initialize_connection != UDTCP_CONNECT_SUCCESS)
    {
        close_tcp_socket(client);
        return (ret_initialize_connection);
    }

    /* replace addr */
    client->server_infos->tcp_addr.sin_addr.s_addr = host_addr;
    client->server_infos->udp_server_addr.sin_addr.s_addr = host_addr;
    client->server_infos->udp_client_addr.sin_addr.s_addr = host_addr;

    /* replace socket */
    client->server_infos->tcp_socket = client->client_infos->tcp_socket;
    client->server_infos->udp_server_socket =
        client->client_infos->udp_client_socket;
    client->server_infos->udp_client_socket =
        client->client_infos->udp_server_socket;

    client->server_infos->send = &(client->send);

    /* copy infos */
    client->server_infos->id = 0;
    udtcp_set_string_infos(client->server_infos,
                           &(client->server_infos->tcp_addr));

    if (client->connect_callback != NULL)
        client->connect_callback(client, client->server_infos);

    return (UDTCP_CONNECT_SUCCESS);
}
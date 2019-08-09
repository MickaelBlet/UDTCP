#include "udtcp.h"

static int accept_all(udtcp_tcp_server_t *in_server)
{
    socklen_t len_addr_client = sizeof(struct sockaddr_in);

    if (in_server->poll_nfds >= UDTCP_MAX_CONNECTION)
    {
        UDTCP_LOG(in_server, 2, "accept(): failed too many connection.\n");
        int sock = accept(in_server->server_infos->socket, NULL, NULL);
        if (sock != -1)
            close(sock);
        return (-1);
    }

    do
    {
        /* accept client */
        in_server->clients_infos[in_server->poll_nfds]->socket = \
            accept(in_server->server_infos->socket, \
                (struct sockaddr*)&(in_server->clients_infos[in_server->poll_nfds]->addr), \
                &len_addr_client);
        if (in_server->clients_infos[in_server->poll_nfds]->socket == -1)
        {
            if (errno != EWOULDBLOCK)
            {
                UDTCP_LOG(in_server, 2, "accept(): failed\n");
                return -1;
            }
            break;
        }

        /* copy infos */
        in_server->clients_infos[in_server->poll_nfds]->id = in_server->count_id;
        const char *hostname = gethostbyaddr( \
            &(in_server->clients_infos[in_server->poll_nfds]->addr.sin_addr), \
            sizeof(struct in_addr), AF_INET)->h_name;
        memcpy(in_server->clients_infos[in_server->poll_nfds]->hostname, hostname, strlen(hostname));
        const char *ip = inet_ntoa(in_server->clients_infos[in_server->poll_nfds]->addr.sin_addr);
        memcpy(in_server->clients_infos[in_server->poll_nfds]->ip, ip, strlen(ip));
        in_server->clients_infos[in_server->poll_nfds]->port = htons(in_server->clients_infos[in_server->poll_nfds]->addr.sin_port);

        ++in_server->count_id;

        /* add in poll table */
        in_server->poll_fds[in_server->poll_nfds].fd = in_server->clients_infos[in_server->poll_nfds]->socket;
        in_server->poll_fds[in_server->poll_nfds].events = POLLIN;

        UDTCP_LOG(in_server, 0, "connect [%lu]%s:%u", \
            in_server->clients_infos[in_server->poll_nfds]->id, \
            in_server->clients_infos[in_server->poll_nfds]->hostname, \
            (int)in_server->clients_infos[in_server->poll_nfds]->port);

        if (in_server->connect_callback != NULL)
            in_server->connect_callback(in_server, in_server->clients_infos[in_server->poll_nfds]);

        ++in_server->poll_nfds;
    } while (in_server->poll_nfds < UDTCP_MAX_CONNECTION);

    return 0;
}

static int receive(udtcp_tcp_server_t *in_server, udtcp_tcp_infos_t *in_client)
{
    uint32_t    data_size;
    int         ret_recv;

    ret_recv = recv(in_client->socket, &data_size, sizeof(uint32_t), 0);
    if (ret_recv < 0)
    {
        return (-1);
    }
    if (ret_recv == 0)
    {
        UDTCP_LOG(in_server, 0, "recv(): close [%lu]%s:%u", \
            in_client->id, \
            in_client->hostname, \
            in_client->port);
        return (0);
    }

    if (ret_recv != sizeof(uint32_t))
    {
        UDTCP_LOG(in_server, 2, "recv(): fail [%lu]%s:%u", \
            in_client->id, \
            in_client->hostname, \
            in_client->port);
        return (-1);
    }

    if (data_size > UDTCP_MAX_DATA_SIZE)
    {
        UDTCP_LOG(in_server, 2, "recv(): size too much");
        return (-1);
    }

    ret_recv = recv(in_client->socket, in_server->buffer_data, data_size, 0);
    if (ret_recv != (int)(data_size))
    {
        UDTCP_LOG(in_server, 2, "recv(): fail [%lu]%s:%u", \
            in_client->id, \
            in_client->hostname, \
            in_client->port);
        return (-1);
    }

    if (in_server->receive_callback != NULL)
    {
        in_server->receive_callback(in_server, in_client, in_server->buffer_data, data_size);
    }
    return (data_size);
}

static void disconnect(udtcp_tcp_server_t *in_server, udtcp_tcp_infos_t *in_client)
{
    UDTCP_LOG(in_server, 0, "disconnect [%lu]%s:%u", \
        in_client->id, \
        in_client->hostname, \
        in_client->port);
    if (in_server->disconnect_callback != NULL)
        in_server->disconnect_callback(in_server, in_client);
    close(in_client->socket);
}

int udtcp_tcp_server_poll(udtcp_tcp_server_t *in_server, long timeout)
{
    size_t i, j;
    int ret_poll;
    int ret_receive;
    int organise_array = 0;
    udtcp_tcp_infos_t *infos;

    ret_poll = poll(in_server->poll_fds, in_server->poll_nfds, timeout);
    if (ret_poll < 0)
    {
        UDTCP_LOG(in_server, 2, "poll(): fail");
        return -1;
    }
    if (ret_poll == 0)
    {
        UDTCP_LOG(in_server, 1, "poll: timeout");
        return -1;
    }

    for (i = 0; i < in_server->poll_nfds; ++i)
    {
        if (in_server->poll_fds[i].revents == 0)
            continue;
        if (in_server->poll_fds[i].revents != POLLIN)
        {
            UDTCP_LOG(in_server, 2, "poll: revent failed");
            return -1;
        }

        /* is server */
        if (in_server->poll_fds[i].fd == in_server->server_infos->socket)
        {
            if (accept_all(in_server) == -1)
                return -1;
        }
        /* is client */
        else
        {
                ret_receive = receive(in_server, in_server->clients_infos[i]);
                if (ret_receive < 0)
                {
                    if (errno != EWOULDBLOCK)
                    {
                        disconnect(in_server, in_server->clients_infos[i]);
                        in_server->poll_fds[i].fd = -1;
                        organise_array = 1;
                    }
                    break;
                }
                else if (ret_receive == 0)
                {
                    disconnect(in_server, in_server->clients_infos[i]);
                    in_server->poll_fds[i].fd = -1;
                    organise_array = 1;
                }
        }
    }
    if (organise_array)
    {
        for (i = 1; i < in_server->poll_nfds; ++i)
        {
            if (in_server->poll_fds[i].fd == -1)
            {
                infos = in_server->clients_infos[i];
                for (j = i; j < in_server->poll_nfds; ++j)
                {
                    in_server->clients_infos[j] = in_server->clients_infos[j + 1];
                    in_server->poll_fds[j].fd = in_server->poll_fds[j + 1].fd;
                }
                /* clean infos */
                memset(infos, 0, sizeof(udtcp_tcp_infos_t));
                in_server->clients_infos[in_server->poll_nfds - 1] = infos;
                --i;
                --in_server->poll_nfds;
            }
        }
    }
    return (0);
}
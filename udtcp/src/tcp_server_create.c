#include "udtcp.h"

static void default_log_callback(udtcp_tcp_server_t *in_server, int level, const char *str)
{
    (void)in_server;
    printf("[%d] %s\n", level, str);
}

static int infos_create(udtcp_tcp_server_t *out_server)
{
    size_t i;

    for (i = 0; i < UDTCP_MAX_CONNECTION; ++i)
    {
        out_server->clients_infos[i] = (udtcp_tcp_infos_t*)malloc(sizeof(udtcp_tcp_infos_t));
        if (out_server->clients_infos[i] == NULL)
        {
            while (i > 0)
            {
                --i;
                free(out_server->clients_infos[i]);
            }
            return (-1);
        }
        memset(out_server->clients_infos[i], 0, sizeof(udtcp_tcp_infos_t));
    }

    return (0);
}

static void infos_destroy(udtcp_tcp_server_t *out_server)
{
    size_t i;

    for (i = 0; i < UDTCP_MAX_CONNECTION; ++i)
        free(out_server->clients_infos[i]);
}

int udtcp_tcp_server_create(const char* in_hostname, uint16_t port, udtcp_tcp_server_t* out_server)
{
    /* clean memory */
    memset(out_server, 0, sizeof(udtcp_tcp_server_t));

    /* allocate infos memory */
    if (infos_create(out_server) == -1)
        return (-1);

    out_server->count_id = 1;

    /* server is the first clients info */
    out_server->server_infos = out_server->clients_infos[0];

    out_server->server_infos->port = port;
    /* create tcp socket */
    out_server->server_infos->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (out_server->server_infos->socket < 0)
    {
        infos_destroy(out_server);
        return (-1);
    }

    out_server->server_infos->addr.sin_family = AF_INET;
    out_server->server_infos->addr.sin_addr.s_addr = inet_addr(in_hostname);
    out_server->server_infos->addr.sin_port = htons(port);

    /* bind port into socket */
    if (bind(out_server->server_infos->socket, (struct sockaddr*)&(out_server->server_infos->addr), sizeof(struct sockaddr_in)) != 0)
    {
        close(out_server->server_infos->socket);
        infos_destroy(out_server);
        return (-1);
    }

    /* active re use addr in socket */
    int ok = 1;
    if (setsockopt(out_server->server_infos->socket, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) == -1)
    {
        close(out_server->server_infos->socket);
        infos_destroy(out_server);
        return (-1);
    }

    /* prepare to accept client */
    if (listen(out_server->server_infos->socket, 1) != 0)
    {
        close(out_server->server_infos->socket);
        infos_destroy(out_server);
        return (-1);
    }

    if (udtcp_noblock_socket(out_server->server_infos->socket, 0) == -1)
    {
        UDTCP_LOG(out_server, 2, "fcntl(): fail");
        infos_destroy(out_server);
        return -1;
    }

    /* copy infos */
    out_server->server_infos->id = 0;
    const char *hostname = gethostbyaddr(&(out_server->server_infos->addr), sizeof(struct in_addr), AF_INET)->h_name;
    memcpy(out_server->server_infos->hostname, hostname, strlen(hostname));
    const char *ip = inet_ntoa(out_server->server_infos->addr.sin_addr);
    memcpy(out_server->server_infos->ip, ip, strlen(ip));

    /* define poll position */
    out_server->poll_nfds = 1;
    out_server->poll_fds[0].fd = out_server->server_infos->socket;
    out_server->poll_fds[0].events = POLLIN;

    /* define default callback */
    out_server->connect_callback = NULL;
    out_server->disconnect_callback = NULL;
    out_server->receive_callback = NULL;
    out_server->log_callback = &default_log_callback;


    return 0;
}
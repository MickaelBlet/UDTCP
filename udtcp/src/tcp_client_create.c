#include "udtcp.h"

static void default_log_callback(udtcp_tcp_client_t *in_client, int level, const char *str)
{
    (void)in_client;
    printf("[%d] %s\n", level, str);
}

int udtcp_tcp_client_create(uint16_t port, udtcp_tcp_client_t *out_client)
{
    /* clean memory */
    memset(out_client, 0, sizeof(udtcp_tcp_client_t));

    out_client->client_infos.port = port;
    /* create tcp socket */
    out_client->client_infos.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( out_client->client_infos.socket < 0)
        return -1;

    if (port > 0)
    {
        out_client->client_infos.addr.sin_family = AF_INET;
        out_client->client_infos.addr.sin_addr.s_addr = INADDR_ANY;
        out_client->client_infos.addr.sin_port = htons(port);

        /* bind port into socket */
        if (bind(out_client->client_infos.socket, (struct sockaddr*)&(out_client->client_infos.addr), sizeof(struct sockaddr_in)) != 0)
        {
            close(out_client->client_infos.socket);
            return -1;
        }
    }

    /* active re use addr in socket */
    int ok = 1;
    if (setsockopt(out_client->client_infos.socket, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) == -1)
    {
        close(out_client->client_infos.socket);
        return (-1);
    }

    if (port > 0)
    {
        /* copy infos */
        out_client->client_infos.id = 0;
        const char *hostname = gethostbyaddr(&(out_client->client_infos.addr.sin_addr), sizeof(struct in_addr), AF_INET)->h_name;
        memcpy(out_client->client_infos.hostname, hostname, strlen(hostname));
        const char *ip = inet_ntoa(out_client->client_infos.addr.sin_addr);
        memcpy(out_client->client_infos.ip, ip, strlen(ip));
    }

    /* define default callback */
    out_client->connect_callback = NULL;
    out_client->disconnect_callback = NULL;
    out_client->receive_callback = NULL;
    out_client->log_callback = &default_log_callback;

    return 0;
}
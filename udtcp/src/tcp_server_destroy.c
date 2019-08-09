#include "udtcp.h"

static void infos_destroy(udtcp_tcp_server_t *out_server)
{
    size_t i;

    for (i = 0; i < UDTCP_MAX_CONNECTION; ++i)
        free(out_server->clients_infos[i]);
}

void udtcp_tcp_server_destroy(udtcp_tcp_server_t *in_server)
{
    size_t i;

    udtcp_tcp_server_poll_break_loop(in_server);

    for (i = in_server->poll_nfds - 1; i > 0; --i)
    {
        if (in_server->poll_fds[i].fd != -1)
            close(in_server->poll_fds[i].fd);
    }
    infos_destroy(in_server);
}
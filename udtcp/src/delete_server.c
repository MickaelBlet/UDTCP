#include <stdlib.h>
#include <unistd.h>

#include "udtcp.h"

void udtcp_delete_server(udtcp_server** addr_server)
{
    size_t i;

    if ((*addr_server)->server_infos->tcp_socket != -1
        && close((*addr_server)->server_infos->tcp_socket) == -1)
        UDTCP_LOG_ERROR((*addr_server), "close: fail");
    if ((*addr_server)->server_infos->udp_server_socket != -1
        && close((*addr_server)->server_infos->udp_server_socket) == -1)
        UDTCP_LOG_ERROR((*addr_server), "close: fail");
    if ((*addr_server)->server_infos->udp_client_socket != -1
        && close((*addr_server)->server_infos->udp_client_socket) == -1)
        UDTCP_LOG_ERROR((*addr_server), "close: fail");
    for (i = 2; i < UDTCP_POLL_TABLE_SIZE; ++i)
        pthread_mutex_destroy(&((*addr_server)->sends[i].mutex));
    for (i = 2; i < (*addr_server)->poll_nfds; ++i)
    {
        if ((*addr_server)->poll_fds[i].fd != -1
            && shutdown((*addr_server)->poll_fds[i].fd, SHUT_RDWR) == -1)
            UDTCP_LOG_ERROR((*addr_server), "close: fail");
        if ((*addr_server)->poll_fds[i].fd != -1
            && close((*addr_server)->poll_fds[i].fd) == -1)
            UDTCP_LOG_ERROR((*addr_server), "close: fail");
        if ((*addr_server)->disconnect_callback != NULL)
            (*addr_server)->disconnect_callback((*addr_server),
                &((*addr_server)->infos[i]));
    }
    free(*addr_server);
    *addr_server = NULL;
}
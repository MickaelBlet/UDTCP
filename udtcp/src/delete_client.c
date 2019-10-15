#include <stdlib.h>
#include <unistd.h>

#include "udtcp.h"

void udtcp_delete_client(udtcp_client** addr_client)
{
    if ((*addr_client)->client_infos->tcp_socket != -1
        && shutdown((*addr_client)->client_infos->tcp_socket, SHUT_RDWR) == -1)
    {
        UDTCP_LOG_ERROR((*addr_client), "shutdown: fail");
    }
    if ((*addr_client)->client_infos->tcp_socket != -1
        && close((*addr_client)->client_infos->tcp_socket) == -1)
    {
        UDTCP_LOG_ERROR((*addr_client), "close: fail");
    }
    if ((*addr_client)->client_infos->udp_server_socket != -1
        && close((*addr_client)->client_infos->udp_server_socket) == -1)
    {
        UDTCP_LOG_ERROR((*addr_client), "close: fail");
    }
    if ((*addr_client)->client_infos->udp_client_socket != -1
        && close((*addr_client)->client_infos->udp_client_socket) == -1)
    {
        UDTCP_LOG_ERROR((*addr_client), "close: fail");
    }
    if ((*addr_client)->client_infos->tcp_socket != -1
        && (*addr_client)->disconnect_callback != NULL)
    {
        (*addr_client)->disconnect_callback((*addr_client),
            (*addr_client)->server_infos);
    }
    if ((*addr_client)->buffer_size > 0)
    {
        free((*addr_client)->buffer_data);
    }
    if ((*addr_client)->send.buffer_size > 0)
    {
        free((*addr_client)->send.buffer);
    }
    pthread_mutex_destroy(&((*addr_client)->send.mutex));
    free(*addr_client);
    *addr_client = NULL;
}
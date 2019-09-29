#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "udtcp.h"

static int buffer_copy(udtcp_infos* infos, const void *data, uint32_t data_size)
{
    uint8_t* new_buffer;

    /* realloc buffer if needed */
    if (infos->send->buffer_size < data_size)
    {
        new_buffer = (uint8_t*)malloc(data_size + sizeof(uint32_t));
        /* malloc error */
        if (new_buffer == NULL)
            return (-1);
        /* delete last buffer */
        if (infos->send->buffer != NULL)
            free(infos->send->buffer);
        /* set new buffer */
        infos->send->buffer = new_buffer;
        infos->send->buffer_size = data_size;
        infos->send->size_offset = infos->send->buffer;
        infos->send->payload_offset =
            infos->send->size_offset + sizeof(uint32_t);
    }

    /* copy data_size and data in send buffer */
    memcpy(infos->send->size_offset, &data_size, sizeof(uint32_t));
    memcpy(infos->send->payload_offset, data, data_size);
    return (0);
}

ssize_t udtcp_send_tcp(udtcp_infos* infos,
    const void *data, uint32_t data_size)
{
    ssize_t ret_send;

    if (pthread_mutex_lock(&(infos->send->mutex)) != 0)
    {
        return (0);
    }
    if (buffer_copy(infos, data, data_size) != 0)
    {
        pthread_mutex_unlock(&(infos->send->mutex));
        return (0);
    }
    ret_send = send(infos->tcp_socket, infos->send->buffer,
        data_size + sizeof(uint32_t), 0);
    pthread_mutex_unlock(&(infos->send->mutex));
    return (ret_send);
}

ssize_t udtcp_send_udp(udtcp_infos* infos,
    const void* data, uint32_t data_size)
{
    ssize_t ret_send;

    if (pthread_mutex_lock(&(infos->send->mutex)) != 0)
    {
        return (0);
    }
    if (buffer_copy(infos, data, data_size) != 0)
    {
        pthread_mutex_unlock(&(infos->send->mutex));
        return (0);
    }
    ret_send = sendto(infos->udp_server_socket, infos->send->buffer,
        data_size + sizeof(uint32_t), 0,
        (struct sockaddr*)&(infos->udp_server_addr),
        sizeof(struct sockaddr_in));
    pthread_mutex_unlock(&(infos->send->mutex));
    return (ret_send);
}
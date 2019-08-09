#include "udtcp.h"

size_t udtcp_tcp_send_formated(const udtcp_tcp_infos_t* infos, const void *in_data, size_t size)
{
    if (size % 2)
        ++size;
    return (send(infos->socket, in_data, size, 0));
}

size_t udtcp_tcp_send(const udtcp_tcp_infos_t* infos, const void *in_data, size_t size)
{
    struct {
        uint32_t size;
        uint8_t buffer[size];
    } data;

    data.size = size;
    memcpy(data.buffer, in_data, size);
    return (send(infos->socket, &data, sizeof(uint32_t) + size, 0));
}
/**
 * udtcp
 *
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019 BLET Mickaël.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "udtcp.h"
#include "udtcp_utils.h"

static int buffer_copy(udtcp_infos* infos, const void *data, uint32_t data_size)
{
    uint8_t* new_buffer;

    /* realloc buffer if needed */
    if (infos->send->buffer_size < data_size)
    {
        new_buffer = udtcp_new_buffer(data_size + sizeof(uint32_t));
        /* malloc error */
        if (new_buffer == NULL)
            return (-1);
        /* delete last buffer */
        if (infos->send->buffer != NULL)
            udtcp_free_buffer(infos->send->buffer);
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

__attribute__((weak))
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

__attribute__((weak))
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
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
#include <unistd.h>

#include "udtcp.h"
#include "udtcp_utils.h"

__attribute__((weak))
void udtcp_delete_client(udtcp_client* client)
{
    if (client->client_infos->tcp_socket != -1
        && shutdown(client->client_infos->tcp_socket, SHUT_RDWR) == -1)
    {
        UDTCP_LOG_ERROR(client, "shutdown: fail");
    }
    if (client->client_infos->tcp_socket != -1
        && close(client->client_infos->tcp_socket) == -1)
    {
        UDTCP_LOG_ERROR(client, "close: fail");
    }
    if (client->client_infos->udp_server_socket != -1
        && close(client->client_infos->udp_server_socket) == -1)
    {
        UDTCP_LOG_ERROR(client, "close: fail");
    }
    if (client->client_infos->udp_client_socket != -1
        && close(client->client_infos->udp_client_socket) == -1)
    {
        UDTCP_LOG_ERROR(client, "close: fail");
    }
    if (client->client_infos->tcp_socket != -1
        && client->disconnect_callback != NULL)
    {
        client->disconnect_callback(client,
            client->server_infos);
    }
    if (client->buffer_size > 0)
    {
        udtcp_free_buffer(client->buffer_data);
    }
    if (client->send.buffer_size > 0)
    {
        udtcp_free_buffer(client->send.buffer);
    }
    pthread_mutex_destroy(&(client->send.mutex));
    udtcp_free_client(client);
}
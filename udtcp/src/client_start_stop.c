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

#include <pthread.h>
#include <unistd.h>
#include <signal.h>         /* SIGTERM */
#include <errno.h>
#include <string.h>

#include "udtcp.h"

static void* thread_client_poll(void* arg)
{
    enum udtcp_poll_e ret_poll;
    udtcp_client* client = arg;

    UDTCP_LOG_INFO(client, "Poll loop start");
    while (client->poll_loop)
    {
        ret_poll = udtcp_client_poll(client, UDTCP_POLL_LOOP_TIMEOUT);
        if (ret_poll == UDTCP_POLL_ERROR || ret_poll == UDTCP_POLL_SIGNAL)
            break;
    }
    client->is_started = 0;
    UDTCP_LOG_INFO(client, "Poll loop end");
    return (NULL);
}

__attribute__((weak))
int udtcp_start_client(udtcp_client* client)
{
    if (client->poll_loop != 0)
    {
        UDTCP_LOG_ERROR(client, "Client is already starting");
        return (-1);
    }
    client->poll_loop = 1;
    client->is_started = 1;
    if (pthread_create(&(client->poll_thread),
        NULL, &thread_client_poll, client) != 0)
    {
        client->poll_loop = 0;
        client->is_started = 0;
        return (-1);
    }
    return (0);
}

__attribute__((weak))
void udtcp_stop_client(udtcp_client* client)
{
    if (client->poll_loop == 0)
    {
        UDTCP_LOG_ERROR(client, "Client is already stopped");
        return;
    }
    client->poll_loop = 0;
    pthread_kill(client->poll_thread, SIGTERM);
    pthread_join(client->poll_thread, NULL);
}
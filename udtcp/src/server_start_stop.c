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
#include <signal.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <bits/sigthread.h>

#include "udtcp.h"

static void* thread_server_poll(void* arg)
{
    enum udtcp_poll_e ret_poll;
    udtcp_server *server = arg;

    UDTCP_LOG_INFO(server, "Poll loop start");
    while (server->poll_loop)
    {
        ret_poll = udtcp_server_poll(server, UDTCP_POLL_LOOP_TIMEOUT);
        if (ret_poll == UDTCP_POLL_ERROR || ret_poll == UDTCP_POLL_SIGNAL)
            break;
    }
    server->is_started = 0;
    UDTCP_LOG_INFO(server, "Poll loop end");
    return NULL;
}

__attribute__((weak))
int udtcp_start_server(udtcp_server* server)
{
    if (server->poll_loop != 0)
    {
        UDTCP_LOG_ERROR(server, "Server is already starting");
        return (-1);
    }
    server->poll_loop = 1;
    if (pthread_create(&(server->poll_thread),
        NULL, &thread_server_poll, server) != 0)
    {
        server->poll_loop = 0;
        return (-1);
    }
    server->is_started = 1;
    return (0);
}

__attribute__((weak))
void udtcp_stop_server(udtcp_server* server)
{
    if (server->poll_loop == 0)
    {
        UDTCP_LOG_ERROR(server, "Server is already stopped");
        return;
    }
    server->poll_loop = 0;
    pthread_kill(server->poll_thread, SIGTERM);
    pthread_join(server->poll_thread, NULL);
}
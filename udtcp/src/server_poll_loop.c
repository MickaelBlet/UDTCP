#include <pthread.h>
#include <signal.h>
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
        if (ret_poll == UDTCP_POLL_ERROR)
        {
            UDTCP_LOG_DEBUG(server, "poll: error");
            break;
        }
        if (ret_poll == UDTCP_POLL_SIGNAL)
        {
            UDTCP_LOG_DEBUG(server, "poll: signal");
            break;
        }
    }
    server->is_started = 0;
    UDTCP_LOG_INFO(server, "Poll loop end");
    return NULL;
}

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
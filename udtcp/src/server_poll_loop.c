#include <signal.h>

#include "udtcp.h"

static void* thread_server_poll(void* arg)
{
    enum udtcp_poll_e ret_poll;
    udtcp_server *server = arg;

    UDTCP_LOG_INFO(server, "Poll loop start");
    server->poll_loop = 1;
    while (server->poll_loop)
    {
        ret_poll = udtcp_server_poll(server, UDTCP_POLL_LOOP_TIMEOUT);
        if (ret_poll == UDTCP_POLL_ERROR || ret_poll == UDTCP_POLL_SIGNAL)
            break;
    }
    UDTCP_LOG_INFO(server, "Poll loop end");
    return NULL;
}

int udtcp_start_server(udtcp_server* server)
{
    return pthread_create(&(server->poll_thread), NULL, &thread_server_poll, server);
}

void udtcp_stop_server(udtcp_server* server)
{
    server->poll_loop = 0;
    pthread_kill(server->poll_thread, SIGTERM);
    pthread_join(server->poll_thread, NULL);
}
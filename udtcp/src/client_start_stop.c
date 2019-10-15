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
        if (ret_poll == UDTCP_POLL_ERROR)
        {
            UDTCP_LOG_DEBUG(client, "poll: error");
            break;
        }
        if (ret_poll == UDTCP_POLL_SIGNAL)
        {
            UDTCP_LOG_DEBUG(client, "poll: signal");
            break;
        }
    }
    client->is_started = 0;
    UDTCP_LOG_INFO(client, "Poll loop end");
    return (NULL);
}

int udtcp_start_client(udtcp_client* client)
{
    if (client->poll_loop != 0)
    {
        UDTCP_LOG_ERROR(client, "Client is already starting");
        return (-1);
    }
    client->poll_loop = 1;
    if (pthread_create(&(client->poll_thread),
        NULL, &thread_client_poll, client) != 0)
    {
        client->poll_loop = 0;
        return (-1);
    }
    client->is_started = 1;
    return (0);
}

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
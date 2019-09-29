#include <signal.h>

#include "udtcp.h"

static void* thread_client_poll(void* arg)
{
    enum udtcp_poll_e ret_poll;
    udtcp_client *client = arg;

    UDTCP_LOG_INFO(client, "Poll loop start");
    client->poll_loop = 1;
    while (client->poll_loop)
    {
        ret_poll = udtcp_client_poll(client, UDTCP_POLL_LOOP_TIMEOUT);
        if (ret_poll == UDTCP_POLL_ERROR || ret_poll == UDTCP_POLL_SIGNAL)
            break;
    }
    UDTCP_LOG_INFO(client, "Poll loop end");
    return NULL;
}

int udtcp_start_client(udtcp_client* client)
{
    return pthread_create(&(client->poll_thread), NULL, &thread_client_poll, client);
}

void udtcp_stop_client(udtcp_client* client)
{
    client->poll_loop = 0;
    pthread_kill(client->poll_thread, SIGTERM);
    pthread_join(client->poll_thread, NULL);
}
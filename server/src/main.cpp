#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream>

#include "udtcp.h"

#define MAX 1
#define PORT 4242

udtcp_tcp_server_t server;

void signalStop(int sig)
{
    (void)sig;
    udtcp_tcp_server_poll_break_loop(&server);
}

void receive_callback(struct udtcp_tcp_server_s* in_server, udtcp_tcp_infos_t *in_infos, void* data, size_t data_size)
{
    (void)in_server;
    (void)in_infos;
    // printf("%lu : %.*s\n", data_size, data_size, (char *)data);
}
// Driver function
int main()
{
    signal(SIGABRT, &signalStop);
    signal(SIGSTOP, &signalStop);
    signal(SIGINT,  &signalStop);
    signal(SIGQUIT, &signalStop);
    signal(SIGPIPE, SIG_IGN);

    if (udtcp_tcp_server_create("0.0.0.0", 2424, &server) == -1)
    {
        return 0;
    }

    server.receive_callback = &receive_callback;

    udtcp_tcp_infos_t *client;
    udtcp_tcp_server_accept(&server, &client, 60000);
    udtcp_tcp_client_t t;
    memset(&t, 0, sizeof(udtcp_tcp_infos_t));
    memcpy(&t.server_infos, client, sizeof(udtcp_tcp_infos_t));
    char *data;
    size_t size;
    std::cout << "receive" << std::endl;
    udtcp_tcp_receive(&t, (void**)&data, &size, 60000);
    char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
    std::cout << "send" << std::endl;
    write(client->socket, hello, strlen(hello));

    std::cout << data << std::endl;

    // udtcp_tcp_server_poll_loop(&server, 60 * 1000);
    udtcp_tcp_server_destroy(&server);

    std::cout << "end server" << std::endl;
}
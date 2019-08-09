#include <iostream>
#include <signal.h>

#include "udtcp.h"

int isRun = 1;

void signalStop(int sig)
{
    (void)sig;
    isRun = 0;
}

int main(int argc, char const *argv[])
{
    signal(SIGABRT, &signalStop);
    signal(SIGSTOP, &signalStop);
    signal(SIGINT,  &signalStop);
    signal(SIGQUIT, &signalStop);
    signal(SIGPIPE, SIG_IGN);

    (void)argc;
    (void)argv;
    int serv_fd = -1;
    const char *hostname = "127.0.0.1";
    int port = 2424;
    long timeout = 10000;
    udtcp_tcp_client_t client;
    int count = 0;
    while (isRun && serv_fd == -1)
    {
        if (udtcp_tcp_client_create(0, &client) == -1)
            std::cerr << "booooouuuuu" << std::endl;

        // serv_fd = udtcp_tcp_client_connect(&client, "216.58.213.131", port, timeout, &server);
        serv_fd = udtcp_tcp_client_connect(&client, hostname, port, timeout);
        if (serv_fd == -1)
        {
            std::cerr << "Retry connection in 2 secondes" << std::endl;
            usleep(2000000);
            count++;
            std::cerr << count << std::endl;
        }
    }
    if (serv_fd != -1)
        std::cout << "Connection success" << std::endl;
    char data[2048];
    while (isRun)
    {
        int i = 0;
        size_t r = 2 + (abs(rand()) % 1024);
        r = r + 8;
        r += r % 8;
        // for (i = 0; i < r; i++)
        // {
        //     data[i] = 97 + rand() % 20;
        // }
        data[r - 1] = '\0';
        size_t ret_send = udtcp_tcp_send(&client.server_infos, data, r) - sizeof(uint32_t);
        if (ret_send == -1)
            break;
        std::cout << ret_send << std::endl;
        // uint8_t *data;
        // size_t data_size;
        // int ret = udtcp_tcp_receive(&client, (void **)&data, &data_size, 2000);
        // if (ret == UDTCP_RECEIVE_FCNTL_ERROR || ret == UDTCP_RECEIVE_ERROR)
        //     break;
        // if (ret == UDTCP_RECEIVE_OK)
        // {
        //     std::cout << (char *)data << " " << data_size << std::endl;
        //     free(data);
        // }
        // usleep(1000000);
    }
    // close(client.server_infos.socket);
    close(client.client_infos.socket);
    return 0;
}
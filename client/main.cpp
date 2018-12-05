#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include "../udtcp/udtcp.h"

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;
    int serv_fd;
    int retConnection = -1;
    while (retConnection == -1)
    {
        retConnection = udtcp_connection("127.0.0.1", 4242, &serv_fd);
        std::cout << "Connection return: " << retConnection << " serv_fd: " << serv_fd << std::endl;
        if (retConnection == -1)
        {
            std::cerr << "Connection to server fail, retry in 2 secondes" << std::endl;
        }
        sleep(2);
    }

    std::cout << "Connection success" << std::endl;
    // server create udp socket
    udtcp_get_udp_port(serv_fd);

    while (42)
    {
        int error = 0;
        socklen_t len = sizeof (error);
        int retval = getsockopt(serv_fd, SOL_SOCKET, SO_KEEPALIVE, &error, &len);
        if (retval != 0)
        {
            /* there was a problem getting the error code */
            fprintf(stderr, "error getting socket error code: %s\n", strerror(retval));
            return 0;
        }

        if (error != 0)
        {
            /* socket has a non zero error status */
            fprintf(stderr, "socket error: %s\n", strerror(error));
        }

        sleep(1);
        write(1, ".", 1);
        char buff[1000];
        if (read(serv_fd, buff, 1000) < 1)
        {
            fprintf(stderr, "serveur disconnect");
            retConnection = -1;
            while (retConnection == -1)
            {
                retConnection = connection("127.0.0.1", 4242, &serv_fd);
                std::cout << "Connection return: " << retConnection << " serv_fd: " << serv_fd << std::endl;
                if (retConnection == -1)
                {
                    std::cerr << "Connection to server fail, retry in 2 secondes" << std::endl;
                }
                sleep(2);
            }
        }
    }
    return 0;
}
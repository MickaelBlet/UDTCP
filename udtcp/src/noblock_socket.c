#include <fcntl.h>

int udtcp_noblock_socket(int socket, int block)
{
    int sock_opt;

    /* get current options of the socket */
    sock_opt = fcntl(socket, F_GETFL, 0);
    if (sock_opt < 0)
        return (-1);

    if (block)
        sock_opt &= (~O_NONBLOCK);
    else
        sock_opt |= O_NONBLOCK;

    /* set new options to the socket */
    if(fcntl(socket, F_SETFL, sock_opt) < 0)
        return (-1);
    return (0);
}
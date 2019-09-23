#include <fcntl.h>

int udtcp_socket_add_option(int socket, int option)
{
    int sock_opt;

    /* get current options of the socket */
    sock_opt = fcntl(socket, F_GETFL, 0);
    if (sock_opt < 0)
        return (-1);

    sock_opt |= option;

    /* set new options to the socket */
    if(fcntl(socket, F_SETFL, sock_opt) < 0)
        return (-1);
    return (0);
}

int udtcp_socket_sub_option(int socket, int option)
{
    int sock_opt;

    /* get current options of the socket */
    sock_opt = fcntl(socket, F_GETFL, 0);
    if (sock_opt < 0)
        return (-1);

    sock_opt &= (~option);

    /* set new options to the socket */
    if(fcntl(socket, F_SETFL, sock_opt) < 0)
        return (-1);
    return (0);
}
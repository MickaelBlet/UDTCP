#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define UDTCP_BUFF_MAX_SIZE 1024

int     udtcp_connection(const char *hostname, int port, int *ret_sock_fd)
{
    struct hostent      *host_entity;
    int                 sock_fd;
    struct sockaddr_in  serv_addr;

    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd == -1)
    {
        perror("socket creation failed");
        return (-1);
    }

    host_entity = gethostbyname(hostname);
    if (host_entity == NULL)
    {
        perror("gethostbyname failed");
        return (-1);
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    memcpy(&serv_addr.sin_addr, host_entity->h_addr_list[0], host_entity->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)))
    {
        perror("connect with server failed");
        return (-1);
    }

    *ret_sock_fd = sock_fd;

    return (0);
}
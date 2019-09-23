#include <arpa/inet.h>  /* inet_ntoa */
#include <netdb.h>      /* gethostbyaddr */
#include <string.h>     /* memcpy, strlen */

#include "udtcp.h"

void udtcp_set_string_infos(udtcp_infos* infos, struct sockaddr_in* addr)
{
    struct hostent* host_entity;
    const char*     ip;
    size_t          ip_lentgh;

    host_entity = gethostbyaddr(addr, sizeof(struct in_addr), AF_INET);
    if (host_entity != NULL)
    {
        memcpy(infos->hostname, host_entity->h_name, strlen(host_entity->h_name));
        ip = inet_ntoa(addr->sin_addr);
        memcpy(infos->ip, ip, strlen(ip));
    }
    else
    {
        ip = inet_ntoa(addr->sin_addr);
        ip_lentgh = strlen(ip);
        memcpy(infos->ip, ip, ip_lentgh);
        memcpy(infos->hostname, ip, ip_lentgh);
    }
}
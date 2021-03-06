/**
 * udtcp
 *
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019 BLET Mickaël.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <arpa/inet.h>  /* inet_ntoa */
#include <netdb.h>      /* gethostbyaddr */
#include <string.h>     /* memcpy, strlen */

#include "udtcp.h"

__attribute__((weak))
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
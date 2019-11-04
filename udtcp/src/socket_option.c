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

#include <fcntl.h>

__attribute__((weak))
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

__attribute__((weak))
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
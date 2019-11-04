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

#ifndef _UDTCP_UTILS_H_
#define _UDTCP_UTILS_H_ 1

#include "udtcp.h"

/*
--------------------------------------------------------------------------------
SOCKET
--------------------------------------------------------------------------------
*/

/**
 * @brief add a option in socket file descriptor
 *
 * @param socket            file descriptor of socket
 * @param option            option to add
 * @return int              zero if success or -1 for error
 */
int udtcp_socket_add_option(int socket, int option);

/**
 * @brief sub a option in socket file descriptor
 *
 * @param socket            file descriptor of socket
 * @param option            option to sub
 * @return int              zero if success or -1 for error
 */
int udtcp_socket_sub_option(int socket, int option);

/*
--------------------------------------------------------------------------------
STRING
--------------------------------------------------------------------------------
*/

/**
 * @brief transform informations of addr to string in infos
 *
 * @param infos             infos to copy string
 * @param addr              sockaddr_in with bits informations
 */
void udtcp_set_string_infos(udtcp_infos* infos, struct sockaddr_in* addr);

/*
--------------------------------------------------------------------------------
NEW - FREE
--------------------------------------------------------------------------------
*/

/**
 * @brief allocate a new buffer
 *
 * @param size              size of buffer
 * @return uint8_t*         new buffer
 */
uint8_t* udtcp_new_buffer(uint32_t size);

/**
 * @brief free a buffer
 *
 * @param buffer
 */
void udtcp_free_buffer(uint8_t* buffer);

/**
 * @brief allocate a new client
 *
 * @return udtcp_client*    new client
 */
udtcp_client* udtcp_new_client(void);

/**
 * @brief free a client
 *
 * @param buffer
 */
void udtcp_free_client(udtcp_client* client);

/**
 * @brief allocate a new server
 *
 * @return udtcp_server*    new server
 */
udtcp_server* udtcp_new_server(void);

/**
 * @brief free a server
 *
 * @param buffer
 */
void udtcp_free_server(udtcp_server* server);

#endif
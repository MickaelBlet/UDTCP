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

#include <stdio.h>
#include <stdlib.h>

#include "udtcp.h"

__attribute__((weak))
uint8_t* udtcp_new_buffer(uint32_t size)
{
    return ((uint8_t*)malloc(size * sizeof(uint8_t)));
}

__attribute__((weak))
void udtcp_free_buffer(uint8_t* buffer)
{
    free(buffer);
}

__attribute__((weak))
udtcp_client* udtcp_new_client(void)
{
    return ((udtcp_client*)malloc(sizeof(udtcp_client)));
}

__attribute__((weak))
void udtcp_free_client(udtcp_client* client)
{
    free(client);
}

__attribute__((weak))
udtcp_server* udtcp_new_server(void)
{
    return ((udtcp_server*)malloc(sizeof(udtcp_server)));
}

__attribute__((weak))
void udtcp_free_server(udtcp_server* server)
{
    free(server);
}
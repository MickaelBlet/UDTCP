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

#ifndef _UDTCP_H_
#define _UDTCP_H_ 1

#include <stdio.h>      /* snprintf */
#include <pthread.h>    /* pthread_mutex_t */
#include <netinet/in.h> /* struct sockaddr_in, uint8_t, uint16_t, uint32_t */
#include <poll.h>       /* struct pollfd */

#ifdef __cplusplus
extern "C" {
#endif

/* enum of log level */
enum udtcp_log_level_e
{
    UDTCP_LOG_LEVEL_ERROR = 0,
    UDTCP_LOG_LEVEL_INFO,
    UDTCP_LOG_LEVEL_DEBUG
};

/* enum of connect return */
enum udtcp_connect_e
{
    UDTCP_CONNECT_SUCCESS = 0,
    UDTCP_CONNECT_ERROR,
    UDTCP_CONNECT_TIMEOUT,
    UDTCP_CONNECT_TOO_MANY
};

/* enum of poll return */
enum udtcp_poll_e
{
    UDTCP_POLL_SUCCESS = 0,
    UDTCP_POLL_ERROR,
    UDTCP_POLL_TIMEOUT,
    UDTCP_POLL_SIGNAL
};

/* define options */
#ifndef UDTCP_MAX_CONNECTION
#define UDTCP_MAX_CONNECTION 42
#endif
#ifndef UDTCP_POLL_LOOP_TIMEOUT
#define UDTCP_POLL_LOOP_TIMEOUT 180000
#endif

/* logs */
#ifdef DEBUG
#define UDTCP_STRINGIFY(x) #x
#define UDTCP_TOSTRING(x) UDTCP_STRINGIFY(x)
#define UDTCP_SOURCE_LOG __FILE__ ":" UDTCP_TOSTRING(__LINE__) " "
#define UDTCP_LOG_DEBUG 1
#define UDTCP_LOG_INFO  1
#define UDTCP_LOG_ERROR 1
#define UDTCP_LOG(udtcp, level, ...)                                        \
    if (udtcp->log_callback != NULL)                                        \
    {                                                                       \
        char out_log[1024];                                                 \
        int prefix_size = snprintf(out_log, 1024, UDTCP_SOURCE_LOG);        \
        snprintf(out_log + prefix_size, 1024 - prefix_size, ##__VA_ARGS__); \
        udtcp->log_callback(udtcp, level, out_log);                         \
    }
#else
#define UDTCP_LOG(udtcp, level, ...)                                        \
    if (udtcp->log_callback != NULL)                                        \
    {                                                                       \
        char out_log[1024];                                                 \
        snprintf(out_log, 1024, ##__VA_ARGS__);                             \
        udtcp->log_callback(udtcp, level, out_log);                         \
    }
#endif


/* if log define replace by macro */
#ifdef UDTCP_LOG_DEBUG
#undef UDTCP_LOG_DEBUG
#define UDTCP_LOG_DEBUG(udtcp, ...)                                        \
    UDTCP_LOG(udtcp, UDTCP_LOG_LEVEL_DEBUG, ##__VA_ARGS__)
#ifndef UDTCP_LOG_INFO
#define UDTCP_LOG_INFO 1
#endif
#else
#define UDTCP_LOG_DEBUG(...) /* nothing */
#endif
#ifdef UDTCP_LOG_INFO
#undef UDTCP_LOG_INFO
#define UDTCP_LOG_INFO(udtcp, ...)                                         \
    UDTCP_LOG(udtcp, UDTCP_LOG_LEVEL_INFO, ##__VA_ARGS__)
#ifndef UDTCP_LOG_ERROR
#define UDTCP_LOG_ERROR 1
#endif
#else
#define UDTCP_LOG_INFO(...) /* nothing */
#endif
#ifdef UDTCP_LOG_ERROR
#undef UDTCP_LOG_ERROR
#define UDTCP_LOG_ERROR(udtcp, ...)                                        \
    UDTCP_LOG(udtcp, UDTCP_LOG_LEVEL_ERROR, ##__VA_ARGS__)
#else
#define UDTCP_LOG_ERROR(...) /* nothing */
#endif

#define UDTCP_POLL_TABLE_SIZE (2 + UDTCP_MAX_CONNECTION)

struct udtcp_send_s
{
    pthread_mutex_t         mutex;
    uint32_t                buffer_size;
    uint8_t*                buffer;
    uint8_t*                size_offset;
    uint8_t*                payload_offset;
};

struct udtcp_infos_s
{
    size_t                  id;
    char                    hostname[256];
    char                    ip[16];
    int                     tcp_socket;
    int                     udp_server_socket;
    int                     udp_client_socket;
    struct sockaddr_in      tcp_addr;
    struct sockaddr_in      udp_server_addr;
    struct sockaddr_in      udp_client_addr;
    uint16_t                tcp_port;
    uint16_t                udp_server_port;
    uint16_t                udp_client_port;
    struct udtcp_send_s*    send;
};

struct udtcp_server_s
{
    struct udtcp_infos_s*   server_infos;
    struct udtcp_infos_s*   clients_infos;
    struct udtcp_infos_s    infos[UDTCP_POLL_TABLE_SIZE];
    struct udtcp_send_s     sends[UDTCP_POLL_TABLE_SIZE];

    struct pollfd           poll_fds[UDTCP_POLL_TABLE_SIZE];
    size_t                  poll_nfds;

    void                    (*connect_callback)
        (struct udtcp_server_s* server, struct udtcp_infos_s* infos);
    void                    (*disconnect_callback)
        (struct udtcp_server_s* server, struct udtcp_infos_s* infos);
    void                    (*receive_tcp_callback)
        (struct udtcp_server_s* server, struct udtcp_infos_s* infos,
         void* data, size_t data_size);
    void                    (*receive_udp_callback)
        (struct udtcp_server_s* server, struct udtcp_infos_s *infos,
         void* data, size_t data_size);
    void                    (*log_callback)
        (struct udtcp_server_s* server,
         enum udtcp_log_level_e level, const char* str);

    size_t                  count_id;

    pthread_t               poll_thread;
    int                     poll_loop;
    volatile int            is_started;

    uint8_t*                buffer_data;
    size_t                  buffer_size;
    void*                   user_data;
};

struct udtcp_client_s
{
    struct udtcp_infos_s*   client_infos;
    struct udtcp_infos_s*   server_infos;
    struct udtcp_infos_s    infos[2];
    struct udtcp_send_s     send;

    struct pollfd           poll_fds[2];
    size_t                  poll_nfds;

    void                    (*connect_callback)
        (struct udtcp_client_s* client, struct udtcp_infos_s* infos);
    void                    (*disconnect_callback)
        (struct udtcp_client_s* client, struct udtcp_infos_s* infos);
    void                    (*receive_tcp_callback)
        (struct udtcp_client_s* client, struct udtcp_infos_s* infos,
         void* data, size_t data_size);
    void                    (*receive_udp_callback)
        (struct udtcp_client_s* client, struct udtcp_infos_s* infos,
         void* data, size_t data_size);
    void                    (*log_callback)
        (struct udtcp_client_s* client,
         enum udtcp_log_level_e level, const char* str);

    pthread_t               poll_thread;
    int                     poll_loop;
    volatile int            is_started;

    uint8_t*                buffer_data;
    size_t                  buffer_size;
    void*                   user_data;
};

typedef struct udtcp_infos_s    udtcp_infos;
typedef struct udtcp_server_s   udtcp_server;
typedef struct udtcp_client_s   udtcp_client;

/*
SOCKET
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
UTILS
*/

/**
 * @brief transform informations of addr to string in infos
 *
 * @param infos
 * @param addr
 */
void udtcp_set_string_infos(udtcp_infos* infos, struct sockaddr_in* addr);

/*
--------------------------------------------------------------------------------
COMMON
--------------------------------------------------------------------------------
*/

/**
 * @brief send in tcp a data
 *
 * @param infos             infos of receiver
 * @param data              data to transmit
 * @param data_size         size of data
 * @return ssize_t          number of bytes sent if success or -1 for error
 */
ssize_t udtcp_send_tcp(udtcp_infos* infos,
    const void* data, uint32_t data_size);

/**
 * @brief send in udp a data to server udp
 *
 * @param infos             infos of receiver
 * @param data              data to transmit
 * @param data_size         size of data
 * @return ssize_t          number of bytes sent if success or -1 for error
 */
ssize_t udtcp_send_udp(udtcp_infos* infos,
    const void* data, uint32_t data_size);

/*
--------------------------------------------------------------------------------
CLIENT
--------------------------------------------------------------------------------
*/

/**
 * @brief create client with tcp and udp server socket and udp client socket
 *
 * @param hostname          define hostname of bind socket
 * @param tcp_port          define tcp port, set zero to get
 *                          an ephemeral port
 * @param udp_server_port   define udp receiver port, set zero to get
 *                          an ephemeral port
 * @param udp_client_port   define udp sender port, set zero to get
 *                          an ephemeral port
 * @param out_server        new udtcp_server_t structure
 * @return int              zero if success or -1 for error
 */
int udtcp_create_client(const char* hostname,
    uint16_t tcp_port, uint16_t udp_server_port, uint16_t udp_client_port,
    udtcp_client** out_client);

/**
 * @brief close all sockets, delete client, set at NULL pointer
 *
 * @param addr_client       address of client
 */
void udtcp_delete_client(udtcp_client** addr_client);

/**
 * @brief connect client to server and transmitting udp information
 *
 * @param client            client pointer
 * @param server_hostname   hostname of server
 * @param server_tcp_port   tcp port of server
 * @param timeout           in milisseconds
 * @return enum udtcp_connect_e
 */
enum udtcp_connect_e udtcp_connect_client(udtcp_client* client,
    const char* server_hostname, uint16_t server_tcp_port, long timeout);

/**
 * @brief poll receive from server sockets
 *
 * @param client            client pointer
 * @param timeout           in milisseconds
 * @return enum udtcp_poll_e
 */
enum udtcp_poll_e udtcp_client_poll(udtcp_client* client, long timeout);

/**
 * @brief start thread poll
 *
 * @param client            client pointer
 * @return int              zero if success or -1 for error
 */
int udtcp_start_client(udtcp_client* client);

/**
 * @brief stop poll thread
 *
 * @param client            client pointer
 */
void udtcp_stop_client(udtcp_client* client);

/*
--------------------------------------------------------------------------------
SERVER
--------------------------------------------------------------------------------
*/

/**
 * @brief create server with tcp and udp server socket and client udp socket
 *
 * @param hostname          define hostname of bind socket
 * @param tcp_port          define tcp port, set zero to get
 *                          an ephemeral port
 * @param udp_server_port   define udp receiver port, set zero to get
 *                          an ephemeral port
 * @param udp_client_port   define udp sender port, set zero to get
 *                          an ephemeral port
 * @param out_server        new udtcp_server_t structure
 * @return int              zero if success or -1 for errors
 */
int udtcp_create_server(const char* hostname,
    uint16_t tcp_port, uint16_t udp_server_port, uint16_t udp_client_port,
    udtcp_server** out_server);

/**
 * @brief close all sockets, delete server, set at NULL pointer
 *
 * @param addr_server       address of server
 */
void udtcp_delete_server(udtcp_server** addr_server);

/**
 * @brief poll accept and receive from client sockets
 *
 * @param server            server pointer
 * @param timeout           in milisseconds
 * @return e_udtcp_poll     zero if success or -1 for errors
 */
enum udtcp_poll_e udtcp_server_poll(udtcp_server* server, long timeout);

/**
 * @brief start poll thread
 *
 * @param server            server pointer
 * @return int              zero if success or -1 for error
 */
int udtcp_start_server(udtcp_server* server);

/**
 * @brief stop poll thread
 *
 * @param server            server pointer
 */
void udtcp_stop_server(udtcp_server* server);

#ifdef __cplusplus
}
#endif

#endif /* _UDTCP_H_ */
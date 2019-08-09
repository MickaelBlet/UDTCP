#ifndef _UDTCP_H_
# define _UDTCP_H_

#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define UDTCP_MAX_CONNECTION    100
#define UDTCP_MAX_DATA_SIZE     8096
#define UDTCP_LOG(udtcp, type, ...)                 \
    if (udtcp->log_callback != NULL) {              \
        char out_log[1024];                         \
        snprintf(out_log, 1024, ##__VA_ARGS__);     \
        udtcp->log_callback(udtcp, type, out_log);  \
    }

#ifdef __cplusplus
extern "C" {
#endif

enum            e_udtcp_log_type
{
    UDTCP_LOG_ERROR,
    UDTCP_LOG_INFO,
    UDTCP_LOG_DEBUG
};

typedef struct          udtcp_tcp_infos_s
{
    size_t              id;
    int                 socket;
    struct sockaddr_in  addr;
    char                hostname[256];
    char                ip[16];
    uint16_t            port;
}                       udtcp_tcp_infos_t;

typedef struct          udtcp_tcp_server_s
{
    udtcp_tcp_infos_t*  server_infos;
    size_t              count_id;
    udtcp_tcp_infos_t*  clients_infos[UDTCP_MAX_CONNECTION];

    struct pollfd       poll_fds[UDTCP_MAX_CONNECTION];
    size_t              poll_nfds;
    int                 poll_loop;

    void                (*connect_callback)(struct udtcp_tcp_server_s* in_server, udtcp_tcp_infos_t *in_infos);
    void                (*disconnect_callback)(struct udtcp_tcp_server_s* in_server, udtcp_tcp_infos_t *in_infos);
    void                (*receive_callback)(struct udtcp_tcp_server_s* in_server, udtcp_tcp_infos_t *in_infos, void* data, size_t data_size);
    void                (*log_callback)(struct udtcp_tcp_server_s* in_server, enum e_udtcp_log_type level, const char* str);

    uint8_t             buffer_data[UDTCP_MAX_DATA_SIZE];
    void*               user_data;
}                       udtcp_tcp_server_t;

typedef struct          udtcp_tcp_client_s
{
    udtcp_tcp_infos_t   client_infos;
    udtcp_tcp_infos_t   server_infos;

    void                (*connect_callback)(struct udtcp_tcp_client_s* in_client, udtcp_tcp_infos_t *in_infos);
    void                (*disconnect_callback)(struct udtcp_tcp_client_s* in_client, udtcp_tcp_infos_t *in_infos);
    void                (*receive_callback)(struct udtcp_tcp_client_s* in_client, udtcp_tcp_infos_t *in_infos, void* data, size_t data_size);
    void                (*log_callback)(struct udtcp_tcp_client_s* in_client, enum e_udtcp_log_type level, const char* str);

    void*               user_data;
}                       udtcp_tcp_client_t;

enum            e_receive
{
    UDTCP_RECEIVE_OK,
    UDTCP_RECEIVE_FCNTL_ERROR,
    UDTCP_RECEIVE_ERROR,
    UDTCP_RECEIVE_TIMEOUT
};

int udtcp_noblock_socket(int socket, int block);

/*
--------------------------------------------------------------------------------
TCP COMMON
--------------------------------------------------------------------------------
*/

size_t udtcp_tcp_send_formated(const udtcp_tcp_infos_t* infos, const void *in_data, size_t size);
size_t udtcp_tcp_send(const udtcp_tcp_infos_t* infos, const void *in_data, size_t size);

/*
--------------------------------------------------------------------------------
TCP CLIENT
--------------------------------------------------------------------------------
*/

enum e_receive udtcp_tcp_receive(udtcp_tcp_client_t* in_tcp, void **out_data, size_t *out_data_size, long timeout);
int udtcp_tcp_client_create(uint16_t port, udtcp_tcp_client_t* out_client);
int udtcp_tcp_client_connect(udtcp_tcp_client_t* in_out_client, const char* in_hostname, uint16_t port, long timeout);

/*
--------------------------------------------------------------------------------
TCP SERVER
--------------------------------------------------------------------------------
*/

int udtcp_tcp_server_create(const char* in_hostname, uint16_t port, udtcp_tcp_server_t* out_server);
int udtcp_tcp_server_accept(udtcp_tcp_server_t* in_server, udtcp_tcp_infos_t** out_client, long timeout);
int udtcp_tcp_server_poll(udtcp_tcp_server_t* in_server, long timeout);
void udtcp_tcp_server_poll_loop(udtcp_tcp_server_t* in_server, long timeout);
void udtcp_tcp_server_poll_break_loop(udtcp_tcp_server_t* in_server);
void udtcp_tcp_server_destroy(udtcp_tcp_server_t *in_server);

/*
--------------------------------------------------------------------------------
UDP SERVER
--------------------------------------------------------------------------------
*/

// int udtcp_udp_server_create(const char *in_hostname, uint16_t port, udtcp_udp_t *out_server);

#ifdef __cplusplus
}
#endif

#endif
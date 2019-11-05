#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "udtcp.h"

#define SERVER_POLL_TIMEOUT (3 * 60 * 1000) /* 3 minutes */

static int g_is_run = 0;
static unsigned int g_count_packet = 0;

static void signal_stop(int sig_number)
{
    (void)sig_number;
    g_is_run = 0;
}

static void ini_signal(void)
{
    struct sigaction sig_action;

    sig_action.sa_handler = &signal_stop;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    /* catch signal term actions */
    sigaction(SIGABRT, &sig_action, NULL);
    sigaction(SIGINT,  &sig_action, NULL);
    sigaction(SIGQUIT, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

static void connect_callback(struct udtcp_server_s* server, udtcp_infos* infos)
{
    (void)server;
    fprintf(stdout,
        "CONNECT CALLBACK "
        "ip: %s, "
        "tcp port: %hu, "
        "udp server port: %hu, "
        "udp client port: %hu\n",
        infos->ip,
        infos->tcp_port,
        infos->udp_server_port,
        infos->udp_client_port);
}

static void disconnect_callback(struct udtcp_server_s* server, udtcp_infos* infos)
{
    (void)server;
    fprintf(stdout,
        "DISCONNECT CALLBACK "
        "ip: %s, "
        "tcp port: %hu, "
        "udp server port: %hu, "
        "udp client port: %hu\n",
        infos->ip,
        infos->tcp_port,
        infos->udp_server_port,
        infos->udp_client_port);
    fprintf(stdout, "number udp packet %u.\n", g_count_packet);
}

static void receive_tcp_callback(struct udtcp_server_s* server, udtcp_infos* infos, void* data, size_t data_size)
{
    (void)server;
    fprintf(stdout,
        "RECEIVE TCP CALLBACK "
        "ip: %s, "
        "tcp port: %hu > [%i] \"%.*s\"\n",
        infos->ip,
        infos->tcp_port,
        (int)infos->id,
        (int)data_size,
        (const char *)data);
}

static void receive_udp_callback(struct udtcp_server_s* server, udtcp_infos* infos, void* data, size_t data_size)
{
    (void)server;
    (void)infos;
    (void)data;
    (void)data_size;
    ++g_count_packet;
    // fprintf(stdout,
    //     "RECEIVE UDP CALLBACK "
    //     "ip: %s, "
    //     "udp port: %hu > [%i] \"%.*s\"\n",
    //     infos->ip,
    //     infos->udp_client_port,
    //     (int)infos->id,
    //     (int)data_size,
    //     (const char *)data);
}

static void log_callback(struct udtcp_server_s* server, enum udtcp_log_level_e level, const char* str)
{
    (void)server;
    static const char *level_to_str[3] = {"ERROR", "INFO ", "DEBUG"};
    fprintf(stdout, "[%s] %s\n", level_to_str[level], str);
}

int main(void)
{
    udtcp_server* server;

    g_is_run = 1;
    ini_signal();

    if (udtcp_create_server("0.0.0.0", 4242, 4242, 4243, &server) == -1)
    {
        fprintf(stderr, "udtcp_create_server: %s\n", strerror(errno));
        return (1);
    }

    /* set callback */
    server->connect_callback = &connect_callback;
    server->disconnect_callback = &disconnect_callback;
    server->receive_tcp_callback = &receive_tcp_callback;
    server->receive_udp_callback = &receive_udp_callback;
    server->log_callback = &log_callback;

    /* create new poll thread */
    udtcp_start_server(server);

    /* wait */
    while (g_is_run && server->is_started)
    {
        sleep(1);
    }

    /* kill and join poll thread */
    udtcp_stop_server(server);

    udtcp_delete_server(server);

    return (0);
}
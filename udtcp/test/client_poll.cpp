#include <gtest/gtest.h>
#include "mock_weak.h"
#include "udtcp.h"
#include "udtcp_utils.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

#define DGTEST_TEST(a, b) GTEST_TEST(a, DISABLED_##b)

// udtcp
void client_receive_tcp_callback(struct udtcp_client_s* client, struct udtcp_infos_s* infos, void* data, size_t data_size);
MOCK_WEAK_DECLTYPE_METHOD4(client_receive_tcp_callback);
void client_disconnect_callback(struct udtcp_client_s* client, struct udtcp_infos_s* infos);
MOCK_WEAK_DECLTYPE_METHOD2(client_disconnect_callback);
MOCK_WEAK_DECLTYPE_METHOD1(udtcp_new_buffer);
MOCK_WEAK_DECLTYPE_METHOD1(udtcp_free_buffer);
// libc
MOCK_WEAK_DECLTYPE_METHOD3(poll);
MOCK_WEAK_DECLTYPE_METHOD4(recv);
MOCK_WEAK_DECLTYPE_METHOD2(shutdown);
MOCK_WEAK_DECLTYPE_METHOD1(close);

GTEST_TEST(udtcp_client_poll, success_simple)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillOnce(Return(1));

    EXPECT_EQ(UDTCP_POLL_SUCCESS, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, timeout)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillOnce(Return(0));

    EXPECT_EQ(UDTCP_POLL_TIMEOUT, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, fail_1)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillRepeatedly(Return(-1));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, fail_2)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLERR;
            return 1;
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, success_tcp_receive_1)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;
    client->receive_tcp_callback = client_receive_tcp_callback;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            char buf[42] = "1234567890123456789012345678901234567890o";
            memcpy(__buf, &buf, 42);
            return 42;
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = EWOULDBLOCK;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    MOCK_WEAK_EXPECT_CALL(client_receive_tcp_callback, (_, _, _, _))
    .WillOnce(Return());

    EXPECT_EQ(UDTCP_POLL_SUCCESS, udtcp_client_poll(client, 42));
    EXPECT_STREQ((char*)client->buffer_data, "1234567890123456789012345678901234567890o");
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, success_tcp_receive_2)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            char buf[12] = "12345678901";
            memcpy(__buf, &buf, 11);
            return 11;
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            char buf[31] = "23456789012345678901234567890o";
            memcpy(__buf, &buf, 31);
            return 31;
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = EWOULDBLOCK;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_SUCCESS, udtcp_client_poll(client, 42));
    EXPECT_STREQ((char*)client->buffer_data, "1234567890123456789012345678901234567890o");
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, success_tcp_receive_3)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = EWOULDBLOCK;
            return -1;
        }
    ));

    EXPECT_EQ(UDTCP_POLL_SUCCESS, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, success_tcp_receive_4)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            uint32_t ft = 11;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            char buf[11] = "1234567890";
            memcpy(__buf, &buf, 11);
            return 11;
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            char buf[42] = "1234567890123456789012345678901234567890o";
            memcpy(__buf, &buf, 42);
            return 42;
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = EWOULDBLOCK;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    MOCK_WEAK_EXPECT_CALL(udtcp_free_buffer, (_))
    .WillOnce(Invoke(
        [](uint8_t* buffer){
            ::free(buffer);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_SUCCESS, udtcp_client_poll(client, 42));
    EXPECT_STREQ((char*)client->buffer_data, "1234567890123456789012345678901234567890o");
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_1)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;
    client->disconnect_callback = client_disconnect_callback;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(client_disconnect_callback, (_, _))
    .WillOnce(Return());

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_2)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            return 0;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_3)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 42;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_4)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            uint32_t value = 666;
            memcpy(__buf, &value, sizeof(uint32_t));
            errno = 0;
            return sizeof(uint32_t);
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Return((uint8_t*)NULL));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_5)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            uint32_t value = 666;
            memcpy(__buf, &value, sizeof(uint32_t));
            return sizeof(uint32_t);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_6)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            uint32_t value = 666;
            memcpy(__buf, &value, sizeof(uint32_t));
            return sizeof(uint32_t);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 0;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_7)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            (void)__fds;
            errno = EINTR;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            errno = 0;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 12;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_8)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            (void)__fds;
            errno = 0;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            errno = 0;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 12;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_9)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            (void)__fds;
            errno = 0;
            return 0;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            errno = 0;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 12;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_10)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            (void)__fds;
            errno = 0;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            errno = 0;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 12;
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return -1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    free(client->buffer_data);
}

GTEST_TEST(udtcp_client_poll, fail_tcp_receive_11)
{
    errno = 0;
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;
    client->client_infos = &client->infos[0];
    client->server_infos = &client->infos[1];
    client->infos[0].tcp_socket = 42;
    client->poll_fds[0].fd = 42;
    client->poll_nfds = 1;
    client->poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, _))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            __fds[0].revents |= POLLIN;
            return 1;
        }
    ))
    .WillOnce(Invoke(
        [](struct pollfd *__fds, nfds_t __nfds, int __timeout){
            (void)__nfds;
            (void)__timeout;
            (void)__fds;
            errno = 0;
            return 1;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(recv, (_, _, _, _))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__n;
            (void)__flags;
            errno = 0;
            uint32_t ft = 42;
            memcpy(__buf, &ft, sizeof(ft));
            return sizeof(ft);
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 12;
        }
    ))
    .WillOnce(Invoke(
        [](int __fd, void *__buf, size_t __n, int __flags){
            (void)__fd;
            (void)__buf;
            (void)__n;
            (void)__flags;
            errno = 0;
            return 0;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(shutdown, (_, _))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(close, (_))
    .WillOnce(Return(-1));

    MOCK_WEAK_EXPECT_CALL(udtcp_new_buffer, (_))
    .WillOnce(Invoke(
        [](uint32_t size){
            return (uint8_t*)::malloc(size);
        }
    ));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    free(client->buffer_data);
}
#include <gtest/gtest.h>
#include "mock_weak.h"

#include "udtcp.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

#define DGTEST_TEST(a, b) GTEST_TEST(a, DISABLED_##b)

// Start
//------------------------------------------------------------------------------

MOCK_WEAK_DECLTYPE_METHOD4(pthread_create);

GTEST_TEST(udtcp_start_client, success)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.is_started = 0;

    MOCK_WEAK_EXPECT_CALL(pthread_create, (_,_,_,_))
    .WillOnce(Return(0));

    EXPECT_EQ(0, udtcp_start_client(&client));
}

// already started
GTEST_TEST(udtcp_start_client, fail1)
{
    udtcp_client client;
    client.poll_loop = 1;
    client.is_started = 0;
    client.log_callback = nullptr;

    EXPECT_EQ(-1, udtcp_start_client(&client));
}

// pthread error
GTEST_TEST(udtcp_start_client, fail2)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.is_started = 0;
    client.log_callback = nullptr;

    MOCK_WEAK_EXPECT_CALL(pthread_create, (_,_,_,_))
    .WillOnce(Return(-1));

    EXPECT_EQ(-1, udtcp_start_client(&client));
}

// Stop
//------------------------------------------------------------------------------

MOCK_WEAK_DECLTYPE_METHOD2(pthread_kill);
MOCK_WEAK_DECLTYPE_METHOD2(pthread_join);

GTEST_TEST(udtcp_stop_client, success)
{
    udtcp_client client;
    client.poll_loop = 1;

    MOCK_WEAK_EXPECT_CALL(pthread_kill, (_,_))
    .WillOnce(Return(0));

    MOCK_WEAK_EXPECT_CALL(pthread_join, (_,_))
    .WillOnce(Return(0));

    udtcp_stop_client(&client);
    EXPECT_EQ(0, client.poll_loop);
}

// already stopped
GTEST_TEST(udtcp_stop_client, fail1)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.log_callback = nullptr;

    udtcp_stop_client(&client);
    EXPECT_EQ(0, client.poll_loop);
}

// Start Stop
//------------------------------------------------------------------------------

MOCK_WEAK_DECLTYPE_METHOD2(udtcp_client_poll);

GTEST_TEST(udtcp_start_client__udtcp_stop_client, end_by_error)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.is_started = 0;
    client.log_callback = nullptr;

    MOCK_WEAK_EXPECT_CALL(udtcp_client_poll, (_,_))
    .WillOnce(Return(UDTCP_POLL_ERROR));

    MOCK_WEAK_EXPECT_CALL(pthread_create, (_,_,_,_))
    .WillOnce(Invoke(
        [](pthread_t * __newthread, const pthread_attr_t * __attr, void *(*__start_routine)(void *), void * __arg){
            (void)__newthread;
            (void)__attr;
            __start_routine(__arg);
            return 0;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(pthread_kill, (_,_))
    .WillOnce(Return(0));

    MOCK_WEAK_EXPECT_CALL(pthread_join, (_,_))
    .WillOnce(Return(0));

    EXPECT_EQ(0, udtcp_start_client(&client));
    EXPECT_EQ(0, client.is_started);
    udtcp_stop_client(&client);
}

GTEST_TEST(udtcp_start_client__udtcp_stop_client, end_by_signal)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.is_started = 0;
    client.log_callback = nullptr;

    MOCK_WEAK_EXPECT_CALL(udtcp_client_poll, (_,_))
    .WillOnce(Return(UDTCP_POLL_SIGNAL));

    MOCK_WEAK_EXPECT_CALL(pthread_create, (_,_,_,_))
    .WillOnce(Invoke(
        [](pthread_t * __newthread, const pthread_attr_t * __attr, void *(*__start_routine)(void *), void * __arg){
            (void)__newthread;
            (void)__attr;
            __start_routine(__arg);
            return 0;
        }
    ));

    MOCK_WEAK_EXPECT_CALL(pthread_kill, (_,_))
    .WillOnce(Return(0));

    MOCK_WEAK_EXPECT_CALL(pthread_join, (_,_))
    .WillOnce(Return(0));

    EXPECT_EQ(0, udtcp_start_client(&client));
    EXPECT_EQ(0, client.is_started);
    udtcp_stop_client(&client);
}
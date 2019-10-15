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

GTEST_TEST(udtcp, start_client_success)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.is_started = 0;

    MOCK_WEAK_EXPECT_CALL(pthread_create, (_,_,_,_))
    .WillOnce(Return(0));

    EXPECT_EQ(0, udtcp_start_client(&client));
}

// already started
GTEST_TEST(udtcp, start_client_fail1)
{
    udtcp_client client;
    client.poll_loop = 1;
    client.is_started = 0;
    client.log_callback = nullptr;

    EXPECT_EQ(-1, udtcp_start_client(&client));
}

// pthread error
GTEST_TEST(udtcp, start_client_fail2)
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

GTEST_TEST(udtcp, stop_client_success)
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
GTEST_TEST(udtcp, stop_client_fail1)
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

static void signalStop(int sig_number)
{
    (void)sig_number;
}

static void ini_signal(void)
{
    struct sigaction sig_action;

    sig_action.sa_handler = &signalStop;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    /* catch signal term actions */
    sigaction(SIGABRT, &sig_action, NULL);
    sigaction(SIGINT,  &sig_action, NULL);
    sigaction(SIGQUIT, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

GTEST_TEST(udtcp, start_stop_client_end_by_error)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.is_started = 0;
    client.log_callback = nullptr;

    MOCK_WEAK_DISABLE(pthread_create);
    MOCK_WEAK_DISABLE(pthread_kill);
    MOCK_WEAK_DISABLE(pthread_join);

    ini_signal();

    MOCK_WEAK_EXPECT_CALL(udtcp_client_poll, (_,_))
    .WillOnce(Return(UDTCP_POLL_ERROR));

    EXPECT_EQ(0, udtcp_start_client(&client));
    usleep(1000);
    EXPECT_EQ(0, client.is_started);
    udtcp_stop_client(&client);
}

GTEST_TEST(udtcp, start_stop_client_end_by_signal)
{
    udtcp_client client;
    client.poll_loop = 0;
    client.is_started = 0;
    client.log_callback = nullptr;

    MOCK_WEAK_DISABLE(pthread_create);
    MOCK_WEAK_DISABLE(pthread_kill);
    MOCK_WEAK_DISABLE(pthread_join);

    ini_signal();

    MOCK_WEAK_EXPECT_CALL(udtcp_client_poll, (_,_))
    .WillOnce(Return(UDTCP_POLL_SIGNAL));

    EXPECT_EQ(0, udtcp_start_client(&client));
    usleep(1000);
    EXPECT_EQ(0, client.is_started);
    udtcp_stop_client(&client);
}
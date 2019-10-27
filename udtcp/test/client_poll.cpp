#include <gtest/gtest.h>
#include "mock_weak.h"

#include "udtcp.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

#define DGTEST_TEST(a, b) GTEST_TEST(a, DISABLED_##b)

MOCK_WEAK_DECLTYPE_METHOD3(poll);

GTEST_TEST(udtcp_client_poll, success_simple)
{
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillOnce(Return(1));

    EXPECT_EQ(UDTCP_POLL_SUCCESS, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, fail_1)
{
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillRepeatedly(Return(-1));

    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
}

GTEST_TEST(udtcp_client_poll, timeout)
{
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    MOCK_WEAK_EXPECT_CALL(poll, (_, _, 42))
    .WillOnce(Return(0));

    EXPECT_EQ(UDTCP_POLL_TIMEOUT, udtcp_client_poll(client, 42));
}

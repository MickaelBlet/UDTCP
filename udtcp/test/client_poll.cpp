#include <gtest/gtest.h>
#include "mock_weak.h"

#include "udtcp.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

#define DGTEST_TEST(a, b) GTEST_TEST(a, DISABLED_##b)

using decltype_poll = decltype(poll);
MOCK_WEAK_METHOD3(poll, decltype_poll);

GTEST_TEST(udtcp, client_poll_fail_1)
{
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    EXPECT_CALL(MOCK_WEAK_INSTANCE(poll), poll(_, _, 42))
    .WillRepeatedly(Return(-1));

    MOCK_WEAK_ENABLE(poll);
    EXPECT_EQ(UDTCP_POLL_ERROR, udtcp_client_poll(client, 42));
    MOCK_WEAK_DISABLE(poll);
}

GTEST_TEST(udtcp, client_poll_timeout)
{
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    EXPECT_CALL(MOCK_WEAK_INSTANCE(poll), poll(_, _, 42))
    .WillOnce(Return(0));

    MOCK_WEAK_ENABLE(poll);
    EXPECT_EQ(UDTCP_POLL_TIMEOUT, udtcp_client_poll(client, 42));
    MOCK_WEAK_DISABLE(poll);
}

GTEST_TEST(udtcp, client_poll_success)
{
    udtcp_client client_instance;
    memset(&client_instance, 0, sizeof(udtcp_client));
    udtcp_client *client = &client_instance;

    EXPECT_CALL(MOCK_WEAK_INSTANCE(poll), poll(_, _, 42))
    .WillOnce(Return(1));

    MOCK_WEAK_ENABLE(poll);
    EXPECT_EQ(UDTCP_POLL_SUCCESS, udtcp_client_poll(client, 42));
    MOCK_WEAK_DISABLE(poll);
}
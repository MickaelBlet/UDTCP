#include <gtest/gtest.h>
#include "mock_weak.h"

#include "udtcp.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

#define DGTEST_TEST(a, b) GTEST_TEST(a, DISABLED_##b)

MOCK_WEAK_DECLTYPE_METHOD3(socket);
MOCK_WEAK_DECLTYPE_METHOD5(setsockopt);
MOCK_WEAK_DECLTYPE_METHOD3(bind);
MOCK_WEAK_DECLTYPE_METHOD3(getsockname);

GTEST_TEST(udtcp_connect_client, success)
{

}
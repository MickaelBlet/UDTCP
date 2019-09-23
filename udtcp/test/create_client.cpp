#include <netdb.h> /* gethostbyname */

#include <gtest/gtest.h>
#include "mock_weak.h"

#include "udtcp.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

#define DGTEST_TEST(a, b) GTEST_TEST(a, DISABLED_##b)

using decltype_gethostbyname = decltype(gethostbyname);
MOCK_WEAK_METHOD1(gethostbyname, decltype_gethostbyname);
using decltype_socket = decltype(socket);
MOCK_WEAK_METHOD3(socket, decltype_socket);
using decltype_setsockopt = decltype(setsockopt);
MOCK_WEAK_METHOD5(setsockopt, decltype_setsockopt);
using decltype_bind = decltype(bind);
MOCK_WEAK_METHOD3(bind, decltype_bind);
using decltype_getsockname = decltype(getsockname);
MOCK_WEAK_METHOD3(getsockname, decltype_getsockname);

GTEST_TEST(create_client, success)
{
    struct hostent instanceHostent;
    char * h_addr_list[1] = {(char *)"\1\0\0\127"};
    instanceHostent.h_addr_list = h_addr_list;
    udtcp_client *out_client;

    MOCK_WEAK_EXPECT_CALL(gethostbyname, gethostbyname("127.0.0.42"))
    .WillOnce(Return((struct hostent *)&instanceHostent));

    MOCK_WEAK_EXPECT_CALL(socket, socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
    .WillRepeatedly(Return(0));
    MOCK_WEAK_EXPECT_CALL(socket, socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
    .WillRepeatedly(Return(0));
    MOCK_WEAK_EXPECT_CALL(setsockopt, setsockopt(_, _, _, _, _))
    .WillRepeatedly(Return(0));
    MOCK_WEAK_EXPECT_CALL(bind, bind(_, _, _))
    .WillRepeatedly(Return(0));
    MOCK_WEAK_EXPECT_CALL(getsockname, getsockname(_, _, _))
    .WillRepeatedly(Return(0));

    EXPECT_EQ(0, udtcp_create_client("127.0.0.42", 4242, 4243, 4444, &out_client));
}
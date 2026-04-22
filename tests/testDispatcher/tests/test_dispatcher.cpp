#include "../../../include/ksockapi.h"
#include "GlobalContext/GlobalContext.hpp"
#include <gtest/gtest.h>

#ifdef _WIN32
static const bool platform = 1;
#else
static const bool platform = 0;
#endif

// Шаблон: <имя проверяемой функции>

TEST(DispatcherTest, net_initialize_test)
{
    globalContext = "NULL";

    std::cout << platform;

    net_initialize();

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_initialize");
    else
        ASSERT_EQ(globalContext, "linux_net_initialize");
}

TEST(DispatcherTest, net_cleanup_test)
{
    globalContext = "NULL";

    net_cleanup();

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_cleanup");
    else
        ASSERT_EQ(globalContext, "linux_net_cleanup");
}

TEST(DispatcherTest, net_socket_create_test)
{
    globalContext = "NULL";

    net_socket_create((net_family_t)0, (net_protocol_t)0, (int)0, (net_socket_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_create");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_create");
}

TEST(DispatcherTest, net_socket_close_test)
{
    globalContext = "NULL";

    net_socket_close((net_socket_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_close");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_close");
}

TEST(DispatcherTest, net_socket_set_options_test)
{
    globalContext = "NULL";

    net_socket_set_options((net_socket_t*)0, (const net_socket_options_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_set_options");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_set_options");
}

TEST(DispatcherTest, net_socket_get_options_test)
{
    globalContext = "NULL";

    net_socket_get_options((net_socket_t*)0, (net_socket_options_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_get_options");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_get_options");
}

TEST(DispatcherTest, net_socket_bind_test)
{
    globalContext = "NULL";

    net_socket_bind((net_socket_t*)0, (const net_address_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_bind");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_bind");
}

TEST(DispatcherTest, net_socket_connect_test)
{
    globalContext = "NULL";

    net_socket_connect((net_socket_t*)0, (const net_address_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_connect");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_connect");
}

TEST(DispatcherTest, net_socket_listen_test)
{
    globalContext = "NULL";

    net_socket_listen((net_socket_t*)0, (int)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_listen");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_listen");
}

TEST(DispatcherTest, net_socket_accept_test)
{
    globalContext = "NULL";

    net_socket_accept((net_socket_t*)0, (net_address_t*)0, (net_socket_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_accept");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_accept");
}

TEST(DispatcherTest, net_socket_send_test)
{
    globalContext = "NULL";

    net_socket_send((net_socket_t*)0, (const void*)0, (size_t)0, (size_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_send");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_send");
}

TEST(DispatcherTest, net_socket_send_to_test)
{
    globalContext = "NULL";

    net_socket_send_to((net_socket_t*)0, (const void*)0, (size_t)0, (const net_address_t*)0, (size_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_send_to");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_send_to");
}

TEST(DispatcherTest, net_socket_receive_test)
{
    globalContext = "NULL";

    net_socket_receive((net_socket_t*)0, (void*)0, (size_t)0, (size_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_receive");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_receive");
}

TEST(DispatcherTest, net_socket_receive_from_test)
{
    globalContext = "NULL";

    net_socket_receive_from((net_socket_t*)0, (void*)0, (size_t)0, (net_address_t*)0, (size_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_receive_from");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_receive_from");
}

TEST(DispatcherTest, net_address_parse_test)
{
    globalContext = "NULL";

    net_address_parse((const char*)0, (uint16_t)0, (net_address_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_address_parse");
    else
        ASSERT_EQ(globalContext, "linux_net_address_parse");
}

TEST(DispatcherTest, net_address_to_string_test)
{
    globalContext = "NULL";

    net_address_to_string((const net_address_t*)0, (char*)0, (size_t)0, (const char*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_address_to_string");
    else
        ASSERT_EQ(globalContext, "linux_net_address_to_string");
}

TEST(DispatcherTest, net_socket_get_local_address_test)
{
    globalContext = "NULL";

    net_socket_get_local_address((net_socket_t*)0, (net_address_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_get_local_address");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_get_local_address");
}

TEST(DispatcherTest, net_socket_get_remote_address_test)
{
    globalContext = "NULL";

    net_socket_get_remote_address((net_socket_t*)0, (net_address_t*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_get_remote_address");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_get_remote_address");
}

TEST(DispatcherTest, net_socket_set_nonblocking_test)
{
    globalContext = "NULL";

    net_socket_set_nonblocking((net_socket_t*)0, (int)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_set_nonblocking");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_set_nonblocking");
}

TEST(DispatcherTest, net_socket_can_read_test)
{
    globalContext = "NULL";

    net_socket_can_read((net_socket_t*)0, (int)0, (int*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_can_read");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_can_read");
}

TEST(DispatcherTest, net_socket_can_write_test)
{
    globalContext = "NULL";

    net_socket_can_write((net_socket_t*)0, (int)0, (int*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_can_write");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_can_write");
}

TEST(DispatcherTest, net_socket_last_error_test)
{
    globalContext = "NULL";

    net_socket_last_error((net_socket_t*)0, (const char*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_socket_last_error");
    else
        ASSERT_EQ(globalContext, "linux_net_socket_last_error");
}

TEST(DispatcherTest, net_error_string_test)
{
    globalContext = "NULL";

    net_error_string((net_error_t)0, (const char*)0);

    if (platform)
        ASSERT_EQ(globalContext, "windows_net_error_string");
    else
        ASSERT_EQ(globalContext, "linux_net_error_string");
}
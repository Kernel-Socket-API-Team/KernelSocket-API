
#include "../../../src/adapters/linux/linux_adapter.h"
#include "GlobalContext/GlobalContext.hpp"

// Sttubs
net_error_t linux_net_initialize(void)
{
    globalContext = "linux_net_initialize";
    return (net_error_t)0;
}

net_error_t linux_net_cleanup(void)
{
    globalContext = "linux_net_cleanup";
    return (net_error_t)0;
}

net_error_t linux_net_socket_create(net_family_t, net_protocol_t, int, net_socket_t*)
{
    globalContext = "linux_net_socket_create";
    return (net_error_t)0;
}

net_error_t linux_net_socket_close(net_socket_t*)
{
    globalContext = "linux_net_socket_close";
    return (net_error_t)0;
}

net_error_t linux_net_socket_set_options(net_socket_t*, const net_socket_options_t*)
{
    globalContext = "linux_net_socket_set_options";
    return (net_error_t)0;
}

net_error_t linux_net_socket_get_options(net_socket_t*, net_socket_options_t*)
{
    globalContext = "linux_net_socket_get_options";
    return (net_error_t)0;
}

net_error_t linux_net_socket_bind(net_socket_t*, const net_address_t*)
{
    globalContext = "linux_net_socket_bind";
    return (net_error_t)0;
}

net_error_t linux_net_socket_connect(net_socket_t*, const net_address_t*)
{
    globalContext = "linux_net_socket_connect";
    return (net_error_t)0;
}

net_error_t linux_net_socket_listen(net_socket_t*, int)
{
    globalContext = "linux_net_socket_listen";
    return (net_error_t)0;
}

net_error_t linux_net_socket_accept(net_socket_t*, net_address_t*, net_socket_t*)
{
    globalContext = "linux_net_socket_accept";
    return (net_error_t)0;
}

net_error_t linux_net_socket_send(net_socket_t*, const void*, size_t, size_t*)
{
    globalContext = "linux_net_socket_send";
    return (net_error_t)0;
}

net_error_t linux_net_socket_send_to(net_socket_t*, const void*, size_t, const net_address_t*, size_t*)
{
    globalContext = "linux_net_socket_send_to";
    return (net_error_t)0;
}

net_error_t linux_net_socket_receive(net_socket_t*, void*, size_t, size_t*)
{
    globalContext = "linux_net_socket_receive";
    return (net_error_t)0;
}

net_error_t linux_net_socket_receive_from(net_socket_t*, void*, size_t, net_address_t*, size_t*)
{
    globalContext = "linux_net_socket_receive_from";
    return (net_error_t)0;
}

net_error_t linux_net_address_parse(const char*, uint16_t, net_address_t*)
{
    globalContext = "linux_net_address_parse";
    return (net_error_t)0;
}

net_error_t linux_net_address_to_string(const net_address_t*, char*, size_t, const char*)
{
    globalContext = "linux_net_address_to_string";
    return (net_error_t)0;
}

net_error_t linux_net_socket_get_local_address(net_socket_t*, net_address_t*)
{
    globalContext = "linux_net_socket_get_local_address";
    return (net_error_t)0;
}

net_error_t linux_net_socket_get_remote_address(net_socket_t*, net_address_t*)
{
    globalContext = "linux_net_socket_get_remote_address";
    return (net_error_t)0;
}

net_error_t linux_net_socket_set_nonblocking(net_socket_t*, int)
{
    globalContext = "linux_net_socket_set_nonblocking";
    return (net_error_t)0;
}

net_error_t linux_net_socket_can_read(net_socket_t*, int, int*)
{
    globalContext = "linux_net_socket_can_read";
    return (net_error_t)0;
}

net_error_t linux_net_socket_can_write(net_socket_t*, int, int*)
{
    globalContext = "linux_net_socket_can_write";
    return (net_error_t)0;
}

net_error_t linux_net_socket_last_error(net_socket_t*, const char*)
{
    globalContext = "linux_net_socket_last_error";
    return (net_error_t)0;
}

net_error_t linux_net_error_string(net_error_t, const char*)
{
    globalContext = "linux_net_error_string";
    return (net_error_t)0;
}
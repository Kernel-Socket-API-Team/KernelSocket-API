#include "../common/linux/linux_common.h"

net_error_t linux_net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type, net_socket_t** socketOut) {
    family = 0;
    protocol = 0;
    type = 0;
    socketOut = 0;
    return 0;
}

net_error_t linux_net_socket_close(net_socket_t* sock) {
    sock = 0;
    return 0;
}

net_error_t linux_net_socket_bind(net_socket_t* sock, const net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t linux_net_socket_connect(net_socket_t* sock, const net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t linux_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent) {
    sock = 0;
    data = 0;
    size = 0;
    sent = 0;
    return 0;
}

net_error_t linux_net_socket_accept(net_socket_t* server, net_socket_t** client_out) {
    server = 0;
    client_out = 0;
    return 0;
}

net_error_t linux_net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received) {
    sock = 0;
    buffer = 0;
    buffer_size = 0;
    from_addr = 0;
    received = 0;
    return 0;
}

net_error_t linux_net_socket_get_address(net_socket_t* sock, net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t linux_net_socket_get_type(net_socket_t* sock, net_socket_type_t* type) {
    sock = 0;
    type = 0;
    return 0;
}

net_error_t linux_net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol) {
    sock = 0;
    protocol = 0;
    return 0;
}

net_error_t linux_net_socket_last_error(net_socket_t *sock, net_error_t* error) {
    sock = 0;
    error = 0;
    return 0;
}

net_error_t linux_net_socket_last_platform_error(net_socket_t* sock, const void** platform_error) {
    sock = 0;
    platform_error = 0;
    return 0;
}
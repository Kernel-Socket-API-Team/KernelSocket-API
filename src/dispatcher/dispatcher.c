#include "../include/ksockapi.h"
#include "dispatcher.h"

#ifdef _WIN32
    #include "../windows/windows_adapter.h"
#else
    #include "../linux/linux_adapter.h"
#endif

// Инициализация указателя на виртуальную таблицу функций
const net_vtable_dispatcher* vtable = NULL;

// Безопастная маршрутизация интерфейса на платформенное определение
net_error_t net_initialize(void) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_initialize)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_initialize();
}

net_error_t net_cleanup(void) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_cleanup)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_cleanup();
}

net_socket_t* net_socket_create(net_family_t family, net_protocol_t protocol, int flags) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_create)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_create(family, protocol, flags);
}

net_error_t net_socket_set_options(net_socket_t* sock, const net_socket_options_t* opts) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_set_options)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_set_options(sock, opts);
}

net_error_t net_socket_get_options(net_socket_t* sock, net_socket_options_t* opts) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_get_options)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_options(sock, opts);
}

net_error_t net_socket_bind(net_socket_t* sock, const net_address_t* addr) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_bind)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_bind(sock, addr);
}

net_error_t net_socket_connect(net_socket_t* sock, const net_address_t* addr) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_connect)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_connect(sock, addr);
}

net_error_t net_socket_listen(net_socket_t* sock, int backlog) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_listen)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_listen(sock, backlog);
}

net_socket_t* net_socket_accept(net_socket_t* sock, net_address_t* client_addr) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_accept)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_accept(sock, client_addr);
}

net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_send)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_send(sock, data, size, sent);
}

net_error_t net_socket_send_to(net_socket_t* sock, const void* data, size_t size, const net_address_t* dest_addr, size_t* sent) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_send_to)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_send_to(sock, data, size, dest_addr, sent);
}

net_error_t net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, size_t* received) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_receive)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_receive(sock, buffer, buffer_size, received);
}

net_error_t* net_socket_receive_from(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* src_addr, size_t* received) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_receive_from)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_receive_from(sock, buffer, buffer_size, src_addr, received);
}

net_error_t net_address_parse(const char* str, uint16_t default_port, net_address_t* addr) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_address_parse)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_address_parse(str, default_port, addr);
}

const char* net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_address_to_string)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_address_to_string(addr, buffer, buffer_size);
}

net_error_t net_socket_get_local_address(net_socket_t* sock, net_address_t* addr) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_get_local_address)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_local_address(sock, addr);
}

net_error_t net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_get_remote_address)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_remote_address(sock, addr);
}

net_error_t net_socket_set_nonblocking(net_socket_t* sock, int enable) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_set_nonblocking)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_set_nonblocking(sock, enable);
}

net_error_t net_socket_can_read(net_socket_t* sock, int timeout_ms, int* can_read) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_can_read)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_can_read(sock, timeout_ms, can_read);
}

net_error_t net_socket_can_write(net_socket_t* sock, int timeout_ms, int* can_write) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_can_write)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_can_write(sock, timeout_ms, can_write);
}

const char* net_socket_last_error(net_socket_t* sock) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_socket_last_error)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_last_error(sock);
}

const char* net_error_string(net_error_t err) {
    if (!vtable)
        return NET_ERROR_NOT_INITIALIZED;
    else if (!vtable->bind_net_error_string)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_error_string(err);
}
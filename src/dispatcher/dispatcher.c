#include "dispatcher.h"

extern const net_vtable_dispatcher windows_vtable;
extern const net_vtable_dispatcher linux_vtable;

// Инициализация указателя на виртуальную таблицу функций
const net_vtable_dispatcher* vtable =
#ifdef _WIN32
    &windows_vtable;
#else
    &linux_vtable;
#endif

// Реализация сокета (скрыта от пользователя)
struct net_socket {
    net_address_t addr;             // Настройки адреса сокета
    net_socket_options_t options;   // Настройки различных опций сокета
    net_error_t error;              // Храним последнюю ошибку, которая возникла при работе с сокетом
    
    void* last_error;               // Храним указатель на последнюю ошибку в контексте конкретной ОС (NTSTATUS ...)
};

// Безопастная маршрутизация интерфейса на платформенное определение
net_error_t net_register(void) {
    if (!vtable || !vtable->bind_net_register)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_register();
}

net_error_t net_activate(const size_t limitMS) {
    if (!vtable || !vtable->bind_net_activate)
        return NET_ERROR_INVALID_VTABLE;
    else 
        return vtable->bind_net_activate(limitMS);
}

net_error_t net_is_ready(void) {
    if (!vtable || !vtable->bind_net_is_ready)
        return NET_ERROR_INVALID_VTABLE;
    else 
        return vtable->bind_net_is_ready();
}

net_error_t net_cleanup(void) {
    if (!vtable || !vtable->bind_net_cleanup)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_cleanup();
}

net_error_t net_socket_create(net_family_t family, net_protocol_t protocol, int flags, net_socket_t* socket_out) {
    if (!vtable || !vtable->bind_net_socket_create)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_create(family, protocol, flags, socket_out);
}

net_error_t net_socket_close(net_socket_t* sock) {
    if (!vtable || !vtable->bind_net_socket_close)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_close(sock);
}

net_error_t net_socket_set_options(net_socket_t* sock, const net_socket_options_t* opts) {
    if (!vtable || !vtable->bind_net_socket_set_options)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_set_options(sock, opts);
}

net_error_t net_socket_get_options(net_socket_t* sock, net_socket_options_t* opts) {
    if (!vtable || !vtable->bind_net_socket_get_options)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_options(sock, opts);
}

net_error_t net_socket_bind(net_socket_t* sock, const net_address_t* addr) {
    if (!vtable || !vtable->bind_net_socket_bind)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_bind(sock, addr);
}

net_error_t net_socket_connect(net_socket_t* sock, const net_address_t* addr) {
    if (!vtable || !vtable->bind_net_socket_connect)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_connect(sock, addr);
}

net_error_t net_socket_listen(net_socket_t* sock, int backlog) {
    if (!vtable || !vtable->bind_net_socket_listen)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_listen(sock, backlog);
}

net_error_t net_socket_accept(net_socket_t* sock, net_address_t* client_addr, net_socket_t* socket_listen) {
    if (!vtable || !vtable->bind_net_socket_accept)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_accept(sock, client_addr, socket_listen);
}

net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent) {
    if (!vtable || !vtable->bind_net_socket_send)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_send(sock, data, size, sent);
}

net_error_t net_socket_send_to(net_socket_t* sock, const void* data, size_t size, const net_address_t* dest_addr, size_t* sent) {
    if (!vtable || !vtable->bind_net_socket_send_to)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_send_to(sock, data, size, dest_addr, sent);
}

net_error_t net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, size_t* received) {
    if (!vtable || !vtable->bind_net_socket_receive)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_receive(sock, buffer, buffer_size, received);
}

net_error_t net_socket_receive_from(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* src_addr, size_t* received) {
    if (!vtable || !vtable->bind_net_socket_receive_from)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_receive_from(sock, buffer, buffer_size, src_addr, received);
}

net_error_t net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr) {
    if (!vtable || !vtable->bind_net_address_parse)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_address_parse(str, ip_family, addr);
}

net_error_t net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port) {
    if (!vtable || !vtable->bind_net_address_to_string)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_address_to_string(addr, buffer, buffer_size, include_port);
}

net_error_t net_socket_get_local_address(net_socket_t* sock, net_address_t* addr) {
    if (!vtable || !vtable->bind_net_socket_get_local_address)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_local_address(sock, addr);
}

net_error_t net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr) {
    if (!vtable || !vtable->bind_net_socket_get_remote_address)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_remote_address(sock, addr);
}

net_error_t net_socket_set_nonblocking(net_socket_t* sock, int enable) {
    if (!vtable || !vtable->bind_net_socket_set_nonblocking)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_set_nonblocking(sock, enable);
}

net_error_t net_socket_can_read(net_socket_t* sock, int timeout_ms, int* can_read) {
    if (!vtable || !vtable->bind_net_socket_can_read)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_can_read(sock, timeout_ms, can_read);
}

net_error_t net_socket_can_write(net_socket_t* sock, int timeout_ms, int* can_write) {
    if (!vtable || !vtable->bind_net_socket_can_write)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_can_write(sock, timeout_ms, can_write);
}

net_error_t net_socket_last_error(net_socket_t* sock, net_error_t error) {
    if (!vtable || !vtable->bind_net_socket_last_error)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_last_error(sock, error);
}

net_error_t net_socket_last_platform_error(net_socket_t* sock, const void* platform_error) {
    if (!vtable || !vtable->bind_net_socket_last_platform_error)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_last_platform_error(sock, platform_error);
}
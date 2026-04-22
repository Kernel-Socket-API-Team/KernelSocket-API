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
struct net_socket
{
    net_protocol_t protocol;   // Тип транспортного протокола (TCP/UDP)
    net_address_t addr;        // Настройки локального адреса сокета
    net_address_t remote_addr; // Настройки удалённого адреса сокета

    net_error_t error; // Храним последнюю ошибку, которая возникла при работе с сокетом
    void* last_error; // Храним указатель на последнюю ошибку в контексте конкретной ОС (NTSTATUS ...)

    net_socket_type_t type; // Тип сокета

    void* context; // Платформозависимый контекст
};

// Безопастная маршрутизация интерфейса на платформенное определение
net_error_t net_register(void)
{
    if (!vtable || !vtable->bind_net_register)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_register();
}

net_error_t net_activate(const size_t limitMS)
{
    if (!vtable || !vtable->bind_net_activate)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_activate(limitMS);
}

net_error_t net_is_ready(void)
{
    if (!vtable || !vtable->bind_net_is_ready)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_is_ready();
}

net_error_t net_cleanup(void)
{
    if (!vtable || !vtable->bind_net_cleanup)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_cleanup();
}

net_error_t net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type,
                              net_socket_t** socketOut)
{
    if (!vtable || !vtable->bind_net_socket_create)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_create(family, protocol, type, socketOut);
}

net_error_t net_socket_close(net_socket_t* sock)
{
    if (!vtable || !vtable->bind_net_socket_close)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_close(sock);
}

net_error_t net_socket_bind(net_socket_t* sock, const net_address_t* addr)
{
    if (!vtable || !vtable->bind_net_socket_bind)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_bind(sock, addr);
}

net_error_t net_socket_connect(net_socket_t* sock, const net_address_t* addr)
{
    if (!vtable || !vtable->bind_net_socket_connect)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_connect(sock, addr);
}

net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent)
{
    if (!vtable || !vtable->bind_net_socket_send)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_send(sock, data, size, sent);
}

net_error_t net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr,
                               size_t* received)
{
    if (!vtable || !vtable->bind_net_socket_receive)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_receive(sock, buffer, buffer_size, from_addr, received);
}

net_error_t net_socket_accept(net_socket_t* server, net_socket_t** client_out)
{
    if (!vtable || !vtable->bind_net_socket_accept)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_accept(server, client_out);
}

net_error_t net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr)
{
    if (!vtable || !vtable->bind_net_address_parse)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_address_parse(str, ip_family, addr);
}

net_error_t net_htons(uint16_t hostshort, uint16_t* netshort)
{
    if (!vtable || !vtable->bind_net_htons)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_htons(hostshort, netshort);
}

net_error_t net_ntohs(uint16_t netshort, uint16_t* hostshort)
{
    if (!vtable || !vtable->bind_net_ntohs)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_ntohs(netshort, hostshort);
}

net_error_t net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port)
{
    if (!vtable || !vtable->bind_net_address_to_string)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_address_to_string(addr, buffer, buffer_size, include_port);
}

net_error_t net_socket_get_address(net_socket_t* sock, net_address_t* addr)
{
    if (!vtable || !vtable->bind_net_socket_get_address)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_address(sock, addr);
}

net_error_t net_socket_get_type(net_socket_t* sock, net_socket_type_t* type)
{
    if (!vtable || !vtable->bind_net_socket_get_type)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_type(sock, type);
}

net_error_t net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol)
{
    if (!vtable || !vtable->bind_net_socket_get_protocol)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_get_protocol(sock, protocol);
}

net_error_t net_socket_last_error(net_socket_t* sock, net_error_t* error)
{
    if (!vtable || !vtable->bind_net_socket_last_error)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_last_error(sock, error);
}

net_error_t net_socket_last_platform_error(net_socket_t* sock, const void** platform_error)
{
    if (!vtable || !vtable->bind_net_socket_last_platform_error)
        return NET_ERROR_INVALID_VTABLE;
    else
        return vtable->bind_net_socket_last_platform_error(sock, platform_error);
}
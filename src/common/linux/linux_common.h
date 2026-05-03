#ifndef COMMON_LINUX_H
#define COMMON_LINUX_H

#include "../../adapters/linux/linux_adapter.h"
#include "../common.h"


// Конвертация ошибок
net_error_t convert_status_from_linux(int error);

// Вспомогательные функции linux_net_address_parse(...)
net_error_t parse_ipv4_address(const char* str, net_address_t* addr);
net_error_t parse_ipv6_address(const char* str, net_address_t* addr);

// Вспомогательные функции linux_net_address_to_string(...)
const char* ipv4_to_string(__be32 addr, char* buffer, size_t size);
const char* ipv6_to_string(const u8* addr, char* buffer, size_t size);

typedef struct LINUX_SOCKET_IMPL
{
    struct socket* kernel_socket; // kernel socket
    struct socket* active_client; // Для TCP клиента
} LINUX_SOCKET_IMPL, *PLINUX_SOCKET_IMPL;

#endif

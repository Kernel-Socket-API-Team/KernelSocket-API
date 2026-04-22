#ifndef COMMON_H
#define COMMON_H

#include "../../include/ksockapi.h"

// Минимальная длина буферов для записи строкового представления адреса
#define NET_IPV4_STR_MIN 8       // "1.1.1.1\0"
#define NET_IPV4_PORT_STR_MIN 10 // "1.1.1.1:1\0"

#define NET_IPV6_STR_MIN 4      // "::1\0"
#define NET_IPV6_PORT_STR_MIN 8 // "[::1]:1\0"

// Максимальная длина буферов для записи строкового представления адреса
#define NET_IPV4_STR_MAX 16      // "255.255.255.255\0"
#define NET_IPV4_PORT_STR_MAX 22 // "255.255.255.255:65535\0"

#define NET_IPV6_STR_MAX 46                  // "ffff:ffff:...:ffff\0"
#define NET_IPV6_PORT_STR_MAX NET_ADDRSTRLEN // "[ffff:...]:65535\0"

// Структура для хранения диапазона буфера
typedef struct net_addres_str_size_t
{
    size_t min;
    size_t max;
} net_addr_str_size_t;

// Функция для определения параметров максимума и минимума для строки
net_addr_str_size_t net_address_required_size(int family, bool include_port);

#endif
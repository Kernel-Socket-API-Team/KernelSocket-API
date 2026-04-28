#ifndef COMMON_LINUX_H
#define COMMON_LINUX_H

#include "../../adapters/linux/linux_adapter.h"
#include "../common.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

// Работа с сокетами
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/net.h>
#include <net/sock.h>

// Вспомогательные заголовочные файлы
#include <linux/slab.h>
#include <linux/string.h>

typedef enum
{
    SOCK_STATE_INIT = 0,      // Только создан
    SOCK_STATE_BOUND = 1,     // Привязан к адресу
    SOCK_STATE_LISTENING = 2, // TCP в режиме прослушивания
    SOCK_STATE_CONNECTED = 3, // TCP подключен (клиент или принятый)
    SOCK_STATE_UDP = 4,       // UDP сокет
} net_socket_state_t;

// Конвертация ошибок
net_error_t convert_status_from_linux(int error);

typedef struct LINUX_SOCKET_IMPL
{
    struct socket* kernel_socket; // kernel socket
} LINUX_SOCKET_IMPL, *PLINUX_SOCKET_IMPL;

#endif
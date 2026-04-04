#ifndef COMMON_WINDOWS_H
#define COMMON_WINDOWS_H

#include "../../adapters/windows/windows_adapter.h"
#include "../common.h"
#include <wsk.h>
#include <wdm.h>

typedef enum {
    SOCK_STATE_INIT        = 0,  // Только создан
    SOCK_STATE_BOUND       = 1,  // Привязан к адресу
    SOCK_STATE_LISTENING   = 2,  // TCP в режиме прослушивания
    SOCK_STATE_CONNECTED   = 3,  // TCP подключен (клиент или принятый)
    SOCK_STATE_UDP         = 4,   // UDP сокет
} net_socket_state_t;

// Глобальный контекст WSK
typedef struct WSK_CONTEXT {
    WSK_REGISTRATION Registration;
    WSK_PROVIDER_NPI ProviderNpi;
    BOOLEAN Initialized;
    BOOLEAN Registered;
} WSK_CONTEXT, *PWSK_CONTEXT;

extern WSK_CONTEXT g_WskContext;
extern WSK_CLIENT_DISPATCH WskAppDispatch;

// Конвертация ошибок
net_error_t convert_status_from_windows(NTSTATUS ntstatus);

// Состояния сокета (упрощенные)
#define SOCK_STATE_INIT       0
#define SOCK_STATE_LISTENING  2
#define SOCK_STATE_CONNECTED  3
#define SOCK_STATE_UDP        4

// Контекст сокета (упрощенный)
typedef struct WINDOWS_SOCKET_IMPL {
    PWSK_SOCKET wsk_socket;         // WSK сокет
    PWSK_SOCKET active_client;      // Для TCP клиента
    KEVENT completion_event;        // Для синхронизации
    BOOLEAN is_listening;           // Режим прослушивания
} WINDOWS_SOCKET_IMPL, *PWINDOWS_SOCKET_IMPL;

#endif
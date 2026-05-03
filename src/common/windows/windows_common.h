#ifndef COMMON_WINDOWS_H
#define COMMON_WINDOWS_H

#include "../../adapters/windows/windows_adapter.h"
#include "../common.h"

// Глобальный контекст WSK
typedef struct WSK_CONTEXT
{
    WSK_REGISTRATION Registration;
    WSK_PROVIDER_NPI ProviderNpi;
    BOOLEAN Initialized;
    BOOLEAN Registered;
} WSK_CONTEXT, *PWSK_CONTEXT;

extern WSK_CONTEXT g_WskContext;
extern WSK_CLIENT_DISPATCH WskAppDispatch;

// Конвертация ошибок
net_error_t convert_status_from_windows(NTSTATUS ntstatus, net_socket_t* sock);

/**
 * Преобразует IPv6 строку с именем интерфейса в строку с числовым scope_id
 *
 * @param str - строка вида "[fe80::1%Ethernet0]" или "[fe80::1%13]"
 * @param output - буфер для выходной строки с числовым scope_id
 * @param output_size - размер буфера
 * @return STATUS_SUCCESS или ошибка
 */
NTSTATUS normalize_ipv6_scope_id(const char* str, char* output, SIZE_T output_size);

// Контекст сокета
typedef struct WINDOWS_SOCKET_IMPL
{
    PWSK_SOCKET wsk_socket;    // WSK сокет
    KEVENT completion_event;   // Для синхронизации
} WINDOWS_SOCKET_IMPL, *PWINDOWS_SOCKET_IMPL;

#endif
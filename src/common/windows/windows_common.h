#ifndef COMMON_WINDOWS_H
#define COMMON_WINDOWS_H

#include "../../adapters/windows/windows_adapter.h"
#include "../common.h"
#include <ntddk.h>
#include <wsk.h>
#include <wdm.h>

/* Контекст для работы с сокетами Windows */
typedef struct WSK_CONTEXT {
    WSK_REGISTRATION Registration;
    WSK_PROVIDER_NPI ProviderNpi;
    KEVENT ProviderReady;
    BOOLEAN Initialized;
    PWORK_QUEUE_ITEM RetryWorkItem;
} WSK_CONTEXT, *PWSK_CONTEXT;

extern WSK_CONTEXT g_WskContext;

/* Диспетчеризация WSK клиента */
const WSK_CLIENT_DISPATCH WskAppDispatch = {
    MAKE_WSK_VERSION(1, 0),     // Версия WSK 1.0
    0,                          // Зарезервировано
    NULL                        // ClientCallback (не используется)
};

/* Рабочий поток для захвата NPI провайдер */
VOID WskCaptureThreadRoutine(PVOID Context);

/* 
    Функция конвертации ошибок Windows под интерфес ошибок,
    который определен в ksockapi.h
    Полный список ошибок можно посмотреть в ntstatus.h
*/
net_error_t convert_status_from_windows(NTSTATUS ntstatus);
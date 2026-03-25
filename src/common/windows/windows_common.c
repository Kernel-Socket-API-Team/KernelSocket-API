#include "windows_common.h"

net_error_t convert_status_from_windows(NTSTATUS ntstatus) {
    if (NT_SUCCESS(ntstatus)) {
        return NET_SUCCESS;
    }
    
    switch (ntstatus) {
        case STATUS_NO_MEMORY:          return NET_ERROR_NO_MEMORY;
        case STATUS_ACCESS_DENIED:      return NET_ERROR_ACCESS_DENIED;
        case STATUS_TIMEOUT:            return NET_ERROR_TIMEOUT;
        case STATUS_BUFFER_TOO_SMALL:   return NET_ERROR_BUFFER_TOO_SMALL;
        case STATUS_INVALID_PARAMETER:  return NET_ERROR_INVALID_PARAM;
        default:                        return NET_ERROR_GENERIC;
    }
}

VOID WskCaptureThreadRoutine(PVOID Context) {
    PWSK_CONTEXT WskContext = (PWSK_CONTEXT)Context;
    NTSTATUS Status;

    while (TRUE) {
        Status = WskCaptureProviderNPI(
            &WskContext->Registration, 
            0,                          // Не ждем ответа, проверяем сразу
            &WskContext->ProviderNpi
        );

        if (NT_SUCCESS(Status)) {
            // Успешно захватили - инициализация завершена
            WskContext->Initialized = TRUE;
            KeSetEvent(&WskContext->ProviderReady, IO_NO_INCREMENT, FALSE);
            break;
        } else if (Status == STATUS_NOINTERFACE) {
            LARGE_INTEGER Delay;
            Delay.QuadPart = -200 * 10000; // 200 мс
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);
        } else {
            // Ошибка
            WskDeregister(&WskContext->Registration);
            KeSetEvent(&WskContext->ProviderReady, IO_NO_INCREMENT, FALSE);
            break;
        }
    }

    // Освобождаем work item
    if (WskContext->RetryWorkItem) {
        ExFreePoolWithTag(&WskContext->RetryWorkItem, 'WSKC');
        WskContext->RetryWorkItem = NULL;
    }
}
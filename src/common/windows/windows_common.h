#ifndef COMMON_WINDOWS_H
#define COMMON_WINDOWS_H

#include "../../adapters/windows/windows_adapter.h"
#include "../common.h"
#include <ntddk.h>
#include <wsk.h>
#include <wdm.h>

/* 
    Функция конвертации ошибок Windows под интерфес ошибок,
    который определен в ksockapi.h
    Полный список ошибок можно посмотреть в ntstatus.h
*/
net_error_t convert_status_from_windows(NTSTATUS ntstatus);

#endif
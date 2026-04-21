#include "windows_common.h"

WSK_CONTEXT g_WskContext = {0}; 

WSK_CLIENT_DISPATCH WskAppDispatch = {
    MAKE_WSK_VERSION(1, 0),     // Версия WSK 1.0
    0,                          // Зарезервировано
    NULL                        // ClientCallback (не используется)
};

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
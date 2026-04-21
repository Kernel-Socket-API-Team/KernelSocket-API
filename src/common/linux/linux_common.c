#include "linux_common.h"

net_error_t convert_status_from_linux(int error) {
   
    if (error >= 0) {
        return NET_SUCCESS;
    }

    int code = -error;

    switch (code) {
        case ENOMEM:        return NET_ERROR_NO_MEMORY;
        case EACCES:        return NET_ERROR_ACCESS_DENIED;
        case EPERM:         return NET_ERROR_ACCESS_DENIED;
        case EINVAL:        return NET_ERROR_INVALID_PARAM;
        case ETIMEDOUT:     return NET_ERROR_TIMEOUT;
        case EMSGSIZE:      return NET_ERROR_BUFFER_TOO_SMALL;
        case ENOBUFS:       return NET_ERROR_NO_MEMORY;

        // Специфические ошибки
        case ECONNREFUSED:
        case EADDRINUSE:
        case ENETUNREACH:
        case EAGAIN:        return NET_ERROR_GENERIC;

        default:            return NET_ERROR_GENERIC;
  }
}
#include "linux_common.h"

LINUX_CONTEXT g_LinuxContext = {
    .Initialized = false,
    .Activated = false
};

net_error_t convert_status_from_linux(int error, net_socket_t* sock)
{
    if (error >= 0)
    {
        if (sock)
        {
            sock->error = NET_SUCCESS;
            sock->last_error = (void*)(long)error;  // сохраняем 0 как успех
        }
        return NET_SUCCESS;
    }

    net_error_t net_error;
    int code = -error;  // Получаем положительный код ошибки

    switch (code)
    {
    case ENOMEM:
        net_error = NET_ERROR_NO_MEMORY;
        break;
    case EACCES:
    case EPERM:
        net_error = NET_ERROR_ACCESS_DENIED;
        break;
    case EINVAL:
        net_error = NET_ERROR_INVALID_PARAM;
        break;
    case ETIMEDOUT:
        net_error = NET_ERROR_TIMEOUT;
        break;
    case EMSGSIZE:
        net_error = NET_ERROR_BUFFER_TOO_SMALL;
        break;
    case ENOBUFS:
        net_error = NET_ERROR_NO_MEMORY;
        break;
    default:
        net_error = NET_ERROR_GENERIC;
        break;
    }

    if (sock)
    {
        sock->error = net_error;
        sock->last_error = (void*)(long)error;  // сохраняем оригинальный код ошибки
    }

    return net_error;
}
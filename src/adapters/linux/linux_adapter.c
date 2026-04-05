#include "linux_adapter.h"

// Инициализаяция виртуальной таблицы функций
const net_vtable_dispatcher linux_vtable = {
    .bind_net_register                      = linux_net_register,
    .bind_net_activate                      = linux_net_activate,
    .bind_net_is_ready                      = linux_net_is_ready,
    .bind_net_cleanup                       = linux_net_cleanup,
    .bind_net_socket_create                 = linux_net_socket_create,
    .bind_net_socket_close                  = linux_net_socket_close,
    .bind_net_socket_bind                   = linux_net_socket_bind,
    .bind_net_socket_connect                = linux_net_socket_connect,
    .bind_net_socket_send                   = linux_net_socket_send,
    .bind_net_socket_receive                = linux_net_socket_receive,
    .bind_net_socket_accept                 = linux_net_socket_accept,
    .bind_net_address_parse                 = linux_net_address_parse,
    .bind_net_htons                         = linux_net_htons,
    .bind_net_ntohs                         = linux_net_ntohs,
    .bind_net_address_to_string             = linux_net_address_to_string,
    .bind_net_socket_get_address            = linux_net_socket_get_address,
    .bind_net_socket_get_type               = linux_net_socket_get_type,
    .bind_net_socket_get_protocol           = linux_net_socket_get_protocol,
    .bind_net_socket_last_error             = linux_net_socket_last_error,
    .bind_net_socket_last_platform_error    = linux_net_socket_last_platform_error,
};
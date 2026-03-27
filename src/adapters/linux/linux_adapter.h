#ifndef LINUX_ADAPTER_H
#define LINUX_ADAPTER_H

#include "../../dispatcher/dispatcher.h"

#ifdef __cplusplus
extern "C" {
#endif

// Интерфейс для Linux реализации
net_error_t     linux_net_register                    (void);
net_error_t     linux_net_activate                    (const size_t);
net_error_t     linux_net_is_ready                    (void);
net_error_t     linux_net_cleanup                     (void);
net_error_t     linux_net_socket_create               (net_family_t, net_protocol_t , int, net_socket_t*);
net_error_t     linux_net_socket_close                (net_socket_t*);
net_error_t     linux_net_socket_set_options          (net_socket_t*, const net_socket_options_t*);
net_error_t     linux_net_socket_get_options          (net_socket_t*, net_socket_options_t*);
net_error_t     linux_net_socket_bind                 (net_socket_t*, const net_address_t*);
net_error_t     linux_net_socket_connect              (net_socket_t*, const net_address_t*);
net_error_t     linux_net_socket_listen               (net_socket_t*, int);
net_error_t     linux_net_socket_accept               (net_socket_t*, net_address_t*, net_socket_t*);
net_error_t     linux_net_socket_send                 (net_socket_t*, const void*, size_t, size_t*);
net_error_t     linux_net_socket_send_to              (net_socket_t*, const void*, size_t, const net_address_t*, size_t*);
net_error_t     linux_net_socket_receive              (net_socket_t*, void*, size_t, size_t*);
net_error_t     linux_net_socket_receive_from         (net_socket_t*, void*, size_t, net_address_t*, size_t*);
net_error_t     linux_net_address_parse               (const char*, net_family_t, net_address_t*);
net_error_t     linux_net_address_to_string           (const net_address_t*, char*, size_t, bool);
net_error_t     linux_net_socket_get_local_address    (net_socket_t*, net_address_t*);
net_error_t     linux_net_socket_get_remote_address   (net_socket_t*, net_address_t*);
net_error_t     linux_net_socket_set_nonblocking      (net_socket_t*, int);
net_error_t     linux_net_socket_can_read             (net_socket_t*, int, int*);
net_error_t     linux_net_socket_can_write            (net_socket_t*, int, int*);
net_error_t     linux_net_socket_last_error           (net_socket_t*, net_error_t);
net_error_t     linux_net_socket_last_platform_error  (net_socket_t*, const void*)

#ifdef __cplusplus
}
#endif

#endif
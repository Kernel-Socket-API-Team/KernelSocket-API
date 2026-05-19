#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "../../include/ksockapi.h"

// Реализация сокета (скрыта от пользователя)
struct net_socket
{
    net_protocol_t protocol;   // Тип транспортного протокола (TCP/UDP)
    net_address_t addr;        // Настройки локального адреса сокета
    net_address_t remote_addr; // Настройки удалённого адреса сокета

    net_error_t error; // Храним последнюю ошибку, которая возникла при работе с сокетом
    void* last_error; // Храним указатель на последнюю ошибку в контексте конкретной ОС (NTSTATUS ...)

    net_socket_type_t type; // Тип сокета

    void* context; // Платформозависимый контекст (не доступен пользователю через геттер)
};

typedef struct
{
    net_error_t (*bind_net_register)(void);
    net_error_t (*bind_net_activate)(const size_t);
    net_error_t (*bind_net_is_ready)(void);
    net_error_t (*bind_net_cleanup)(void);
    net_error_t (*bind_net_socket_create)(net_family_t, net_protocol_t, net_socket_type_t, net_socket_t**);
    net_error_t (*bind_net_socket_close)(net_socket_t*);
    net_error_t (*bind_net_socket_bind)(net_socket_t*, const net_address_t*);
    net_error_t (*bind_net_socket_connect)(net_socket_t*, const net_address_t*);
    net_error_t (*bind_net_socket_send)(net_socket_t*, const void*, size_t, size_t*);
    net_error_t (*bind_net_socket_receive)(net_socket_t*, void*, size_t, net_address_t*, size_t*);
    net_error_t (*bind_net_socket_accept)(net_socket_t*, net_socket_t**);
    net_error_t (*bind_net_address_parse)(const char*, net_family_t, net_address_t*);
    net_error_t (*bind_net_htons)(uint16_t, uint16_t*);
    net_error_t (*bind_net_ntohs)(uint16_t, uint16_t*);
    net_error_t (*bind_net_address_to_string)(const net_address_t*, char*, size_t, bool);
    net_error_t (*bind_net_socket_get_address)(net_socket_t*, net_address_t*);
    net_error_t (*bind_net_socket_get_remote_address)(net_socket_t*, net_address_t*); 
    net_error_t (*bind_net_socket_get_type)(net_socket_t*, net_socket_type_t*);
    net_error_t (*bind_net_socket_get_protocol)(net_socket_t*, net_protocol_t*);
    net_error_t (*bind_net_socket_last_error)(net_socket_t*, net_error_t*);
    net_error_t (*bind_net_socket_last_platform_error)(net_socket_t*, const void**);
} net_vtable_dispatcher;

extern const net_vtable_dispatcher* vtable;

#endif
#include "../common/windows/windows_common.h"

net_error_t windows_net_socket_create(net_family_t family, net_protocol_t protocol, int flags, net_socket_t* socket_out);

net_error_t windows_net_socket_close(net_socket_t* sock);

net_error_t windows_net_socket_set_options(net_socket_t* sock, const net_socket_options_t* opts);

net_error_t windows_net_socket_get_options(net_socket_t* sock, net_socket_options_t* opts);

net_error_t windows_net_socket_bind(net_socket_t* sock, const net_address_t* addr);

net_error_t windows_net_socket_connect(net_socket_t* sock, const net_address_t* addr);

net_error_t windows_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);

// ============ windows_net_socket.c ============

net_error_t windows_net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received) {
    NTSTATUS status;
    PWSK_BUF wsk_buf;
    SIZE_T bytes_received = 0;
    
    // Проверка параметров
    if (!sock || !buffer || buffer_size == 0) {
        return NET_ERROR_INVALID_PARAM;
    }
    
    // Инициализация выходных параметров
    if (received) {
        *received = 0;
    }
    if (from_addr) {
        RtlZeroMemory(from_addr, sizeof(net_address_t));
    }
    
    // Получаем Windows-контекст
    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl || !impl->wsk_socket) {
        sock->error = NET_ERROR_INVALID_STATE;
        return NET_ERROR_INVALID_STATE;
    }
    
    // ============ TCP ============
    if (sock->protocol == NET_PROTO_TCP) {
        
        // LISTEN с backlog=1
        if (!impl->is_listening) {
            status = WskListen(impl->wsk_socket, 0, 1);  // backlog = 1
            if (!NT_SUCCESS(status)) {
                sock->last_error = (void*)status;
                sock->error = convert_status_from_windows(status);
                return sock->error;
            }
            impl->is_listening = TRUE;
            sock->state = SOCK_STATE_LISTENING;
        }
        
        // Если нет активного соединения - принимаем новое
        if (!impl->active_client) {
            SOCKADDR_INET client_addr;
            SIZE_T addr_size = sizeof(client_addr);
            PWSK_SOCKET client_wsk_sock;
            
            status = WskAccept(impl->wsk_socket, 0, &client_addr, &addr_size, NULL, NULL, &client_wsk_sock);
            
            if (!NT_SUCCESS(status)) {
                sock->last_error = (void*)status;
                sock->error = convert_status_from_windows(status);
                return sock->error;
            }
            
            // Сохраняем клиентский сокет в контексте
            impl->active_client = client_wsk_sock;
            
            // Заполняем адрес клиента (если нужно)
            if (from_addr) {
                if (client_addr.si_family == AF_INET) {
                    from_addr->family = NET_AF_INET4;
                    from_addr->port = client_addr.Ipv4.sin_port;
                    from_addr->addr.ipv4 = client_addr.Ipv4.sin_addr.s_addr;
                } else {
                    from_addr->family = NET_AF_INET6;
                    from_addr->port = client_addr.Ipv6.sin6_port;
                    RtlCopyMemory(from_addr->addr.ipv6, client_addr.Ipv6.sin6_addr.u.Byte, 16);
                }
            }
        }
        
        // Принимаем данные от активного клиента
        wsk_buf = (PWSK_BUF)ExAllocatePoolWithTag(NonPagedPool, sizeof(WSK_BUF), 'buSW');
        if (!wsk_buf) {
            return NET_ERROR_NO_MEMORY;
        }
        
        wsk_buf->Mdl = IoAllocateMdl(buffer, (ULONG)buffer_size, FALSE, FALSE, NULL);
        if (!wsk_buf->Mdl) {
            ExFreePoolWithTag(wsk_buf, 'buSW');
            return NET_ERROR_NO_MEMORY;
        }
        
        MmBuildMdlForNonPagedPool(wsk_buf->Mdl);
        wsk_buf->Offset = 0;
        wsk_buf->Length = buffer_size;
        
        KeResetEvent(&impl->completion_event);
        status = WskReceive(impl->active_client, wsk_buf, 0, &bytes_received,
                           NULL, NULL, NULL);
        
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&impl->completion_event, Executive, KernelMode, FALSE, NULL);
            status = impl->last_status;
        }
        
        // Очистка
        IoFreeMdl(wsk_buf->Mdl);
        ExFreePoolWithTag(wsk_buf, 'buSW');
        
        if (!NT_SUCCESS(status)) {
            // Ошибка приема - закрываем соединение
            WskCloseSocket(impl->active_client);
            impl->active_client = NULL;
            sock->error = convert_status_from_windows(status);
            return sock->error;
        }
        
        if (received) {
            *received = bytes_received;
        }
        
        // Если данные не получены (соединение закрыто клиентом)
        if (bytes_received == 0) {
            WskCloseSocket(impl->active_client);
            impl->active_client = NULL;
        }
        
        return NET_SUCCESS;
    }
    
    // ============ UDP ============
    if (sock->protocol == NET_PROTO_UDP) {
        
        // UDP требует адрес отправителя
        if (!from_addr) {
            return NET_ERROR_INVALID_PARAM;
        }
        
        wsk_buf = (PWSK_BUF)ExAllocatePoolWithTag(NonPagedPool, sizeof(WSK_BUF), 'buSW');
        if (!wsk_buf) {
            return NET_ERROR_NO_MEMORY;
        }
        
        wsk_buf->Mdl = IoAllocateMdl(buffer, (ULONG)buffer_size, FALSE, FALSE, NULL);
        if (!wsk_buf->Mdl) {
            ExFreePoolWithTag(wsk_buf, 'buSW');
            return NET_ERROR_NO_MEMORY;
        }
        
        MmBuildMdlForNonPagedPool(wsk_buf->Mdl);
        wsk_buf->Offset = 0;
        wsk_buf->Length = buffer_size;
        
        SOCKADDR_INET sender_addr;
        SIZE_T addr_size = sizeof(sender_addr);
        
        KeResetEvent(&impl->completion_event);
        status = WskReceiveFrom(impl->wsk_socket, wsk_buf, 0, &bytes_received,
                               &sender_addr, &addr_size, NULL, NULL);
        
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&impl->completion_event,
                                 Executive, KernelMode, FALSE, NULL);
            status = impl->last_status;
        }
        
        IoFreeMdl(wsk_buf->Mdl);
        ExFreePoolWithTag(wsk_buf, 'buSW');
        
        if (!NT_SUCCESS(status)) {
            sock->error = convert_status_from_windows(status);
            return sock->error;
        }
        
        if (received) {
            *received = bytes_received;
        }
        
        // Заполняем адрес отправителя
        if (sender_addr.si_family == AF_INET) {
            from_addr->family = NET_AF_INET4;
            from_addr->port = sender_addr.Ipv4.sin_port;
            from_addr->addr.ipv4 = sender_addr.Ipv4.sin_addr.s_addr;
        } else {
            from_addr->family = NET_AF_INET6;
            from_addr->port = sender_addr.Ipv6.sin6_port;
            RtlCopyMemory(from_addr->addr.ipv6, sender_addr.Ipv6.sin6_addr.u.Byte, 16);
        }
        
        return NET_SUCCESS;
    }
    
    return NET_ERROR_INVALID_PROTOCOL;
}

/*
net_error_t net_socket_listen(net_socket_t* sock, int backlog)
{
    NTSTATUS status;
    
    if (!sock) {
        return NET_ERROR_INVALID_PARAM;
    }
    
    // Проверяем протокол
    if (sock->protocol != NET_PROTO_TCP) {
        sock->error = NET_ERROR_INVALID_PROTOCOL;
        return NET_ERROR_INVALID_PROTOCOL;
    }
    
    // Получаем Windows-контекст
    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl || !impl->wsk_socket) {
        sock->error = NET_ERROR_INVALID_STATE;
        return NET_ERROR_INVALID_STATE;
    }
    
    // Проверка, что сокет привязан
    if (!impl->is_bound) {
        sock->error = NET_ERROR_INVALID_STATE;
        return NET_ERROR_INVALID_STATE;
    }
    
    // Уже в режиме прослушивания?
    if (sock->state == SOCK_STATE_LISTENING) {
        return NET_SUCCESS;
    }
    
    // Вызов WskListen
    status = WskListen(impl->wsk_socket, 0, (ULONG)backlog);
    
    if (!NT_SUCCESS(status)) {
        sock->last_error = (void*)status;
        sock->error = convert_status_from_windows(status);
        return sock->error;
    }
    
    sock->state = SOCK_STATE_LISTENING;
    
    return NET_SUCCESS;
}*/

net_error_t windows_net_socket_get_local_address(net_socket_t* sock, net_address_t* addr);

net_error_t windows_net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr);

net_error_t windows_net_socket_set_nonblocking(net_socket_t* sock, int enable);

// Данная функция требует тестов!!!!
net_error_t windows_net_socket_last_error(net_socket_t *sock, net_error_t error) {
    if (!sock)
        return NET_ERROR_INVALID_PARAM;
    
    error = sock->error;

    return NET_SUCCESS;
}

// Данная функция требует тестов!!!!
net_error_t windows_net_socket_last_platform_error(net_socket_t* sock, const void* platform_error) {
    if (!sock)
        return NET_ERROR_INVALID_PARAM;

    platform_error = sock->last_error;

    return NET_SUCCESS;
}

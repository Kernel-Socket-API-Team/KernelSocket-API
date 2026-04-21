#include "../common/windows/windows_common.h"

static NTSTATUS wsk_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceObject);

    PKEVENT event = (PKEVENT)Context;

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

net_error_t windows_net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type, net_socket_t** socketOut) {
    
    net_error_t lib_state = windows_net_is_ready();
    if (lib_state != NET_SUCCESS)
        return lib_state;

    if (!socketOut) 
        return NET_ERROR_INVALID_PARAM;
    
    if (protocol == NET_PROTO_UDP && type != NET_SOCK_TYPE_UDP) 
        return NET_ERROR_INVALID_PARAM;

    if (protocol == NET_PROTO_TCP && type == NET_SOCK_TYPE_UDP)
        return NET_ERROR_INVALID_PARAM;
    
    if (type != NET_SOCK_TYPE_TCP_LISTEN && 
        type != NET_SOCK_TYPE_TCP_CONNECTION && 
        type != NET_SOCK_TYPE_UDP) 
        return NET_ERROR_INVALID_PARAM;

        
    NTSTATUS status;
    PWSK_SOCKET wsk_socket;
    PWINDOWS_SOCKET_IMPL impl;
    net_socket_t* sock;
    PIRP irp;
    KEVENT event;

    ULONG wsk_flags = 0;

    switch (type) {
    case NET_SOCK_TYPE_TCP_LISTEN:
        wsk_flags = WSK_FLAG_LISTEN_SOCKET;
        break;

    case NET_SOCK_TYPE_TCP_CONNECTION:
        wsk_flags = WSK_FLAG_CONNECTION_SOCKET;
        break;

    case NET_SOCK_TYPE_UDP:
        wsk_flags = WSK_FLAG_DATAGRAM_SOCKET;
        break;

    default:
        return NET_ERROR_INVALID_PARAM;
    }
    
    // Создаем сокет
    sock = (net_socket_t*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(net_socket_t), 'kcoS');
    if (!sock) return NET_ERROR_NO_MEMORY;
    
    RtlZeroMemory(sock, sizeof(net_socket_t));
    
    // Определяем параметры WSK сокета
    USHORT wsk_family = (family == NET_AF_INET4) ? AF_INET : AF_INET6;
    USHORT wsk_type = (protocol == NET_PROTO_TCP) ? SOCK_STREAM : SOCK_DGRAM;
    ULONG wsk_protocol = (protocol == NET_PROTO_TCP) ? IPPROTO_TCP : IPPROTO_UDP;
    
    irp = IoAllocateIrp(1, FALSE);
    if (!irp) {
        ExFreePool(sock);
        return NET_ERROR_NO_MEMORY;
    }
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);
    
    status = g_WskContext.ProviderNpi.Dispatch->WskSocket(
        g_WskContext.ProviderNpi.Client,  // Client
        wsk_family,                       // AddressFamily
        wsk_type,                         // SocketType
        wsk_protocol,                     // Protocol
        wsk_flags,                        // Flags
        NULL,                             // SocketContext
        &WskAppDispatch,                  // Dispatch
        NULL,                             // OwningProcess
        NULL,                             // OwningThread
        NULL,                             // SecurityDescriptor
        irp                               // Irp
    );
    
    // Ожидаем завершения операции
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }
    
    if (NT_SUCCESS(status)) {
        // Сокет возвращается через IoStatus.Information
        wsk_socket = (PWSK_SOCKET)irp->IoStatus.Information;
    } else {
        IoFreeIrp(irp);
        ExFreePool(sock);
        return convert_status_from_windows(status);
    }
    
    IoFreeIrp(irp);
    
    // Создаем Windows контекст
    impl = (PWINDOWS_SOCKET_IMPL)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(WINDOWS_SOCKET_IMPL), 'pmI');
    if (!impl) {
        ((PWSK_PROVIDER_BASIC_DISPATCH)wsk_socket->Dispatch)->WskCloseSocket(wsk_socket, NULL);
        ExFreePool(sock);
        return NET_ERROR_NO_MEMORY;
    }
    
    RtlZeroMemory(impl, sizeof(WINDOWS_SOCKET_IMPL));
    impl->wsk_socket = wsk_socket;
    KeInitializeEvent(&impl->completion_event, NotificationEvent, FALSE);
    
    sock->addr.family = family;
    sock->context = impl;
    sock->protocol = protocol;
    sock->type = type;
    
    *socketOut = sock;
    
    return NET_SUCCESS;
}

net_error_t windows_net_socket_close(net_socket_t* sock) {
    if (!sock) 
        return NET_ERROR_INVALID_PARAM;    
    
    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl || !impl->wsk_socket) 
        return NET_ERROR_INVALID_STATE;
    
    // Сохраняем указатель на wsk_socket перед очисткой
    PWSK_SOCKET wsk = impl->wsk_socket;
    PWSK_SOCKET active = impl->active_client;
    
    // Закрываем клиентский сокет (если есть)
    if (active && active != wsk) {
        PIRP irp = IoAllocateIrp(1, FALSE);
        if (irp) {
            KEVENT event;
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);
            
            ((PWSK_PROVIDER_BASIC_DISPATCH)active->Dispatch)->WskCloseSocket(active, irp);
            
            if (irp->IoStatus.Status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            }
            IoFreeIrp(irp);
        }
    }
    
    // Закрываем серверный сокет - прерывает все блокирующие функции сокета
    if (wsk) {
        PIRP irp = IoAllocateIrp(1, FALSE);
        if (irp) {
            KEVENT event;
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);
            
            ((PWSK_PROVIDER_BASIC_DISPATCH)wsk->Dispatch)->WskCloseSocket(wsk, irp);
            
            if (irp->IoStatus.Status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            }
            IoFreeIrp(irp);
        }
    }
    
    if (impl) ExFreePool(impl);

    ExFreePool(sock);
    
    return NET_SUCCESS;
}

net_error_t windows_net_socket_bind(net_socket_t* sock, const net_address_t* addr) {
    if (!sock || !addr) 
        return NET_ERROR_INVALID_PARAM;
    
    if (sock->addr.family != addr->family) 
        return NET_ERROR_INVALID_PARAM;

    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl || !impl->wsk_socket) 
        return NET_ERROR_INVALID_STATE;
    
    PIRP irp = IoAllocateIrp(1, FALSE);
    if (!irp) 
        return NET_ERROR_NO_MEMORY;
    
    KEVENT event;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);
    
    NTSTATUS status;
    SOCKADDR_STORAGE local_addr_storage;
    PSOCKADDR pSockAddr = NULL;
    ULONG addr_size = 0;
    
    // Подготовка адреса в зависимости от семейства
    if (addr->family == NET_AF_INET4) {
        PSOCKADDR_IN pAddr4 = (PSOCKADDR_IN)&local_addr_storage;
        RtlZeroMemory(pAddr4, sizeof(SOCKADDR_IN));
        pAddr4->sin_family = AF_INET;
        pAddr4->sin_port = addr->port;
        pAddr4->sin_addr.s_addr = addr->addr.ipv4;
        pSockAddr = (PSOCKADDR)pAddr4;
        addr_size = sizeof(SOCKADDR_IN);
    }
    else if (addr->family == NET_AF_INET6) {
        PSOCKADDR_IN6 pAddr6 = (PSOCKADDR_IN6)&local_addr_storage;
        RtlZeroMemory(pAddr6, sizeof(SOCKADDR_IN6));
        pAddr6->sin6_family = AF_INET6;
        pAddr6->sin6_port = addr->port;
        RtlCopyMemory(&pAddr6->sin6_addr, addr->addr.ipv6, 16);
        pAddr6->sin6_flowinfo = 0;
        pAddr6->sin6_scope_id = 0;
        pSockAddr = (PSOCKADDR)pAddr6;
        addr_size = sizeof(SOCKADDR_IN6);
    }
    else {
        IoFreeIrp(irp);
        return NET_ERROR_INVALID_PARAM;
    }
    
    switch (sock->type) {
        case NET_SOCK_TYPE_TCP_LISTEN:
            status = ((PWSK_PROVIDER_LISTEN_DISPATCH)impl->wsk_socket->Dispatch)
                ->WskBind(impl->wsk_socket, pSockAddr, 0, irp);
            break;
            
        case NET_SOCK_TYPE_TCP_CONNECTION:
            status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)impl->wsk_socket->Dispatch)
                ->WskBind(impl->wsk_socket, pSockAddr, 0, irp);
            break;
            
        case NET_SOCK_TYPE_UDP:
            status = ((PWSK_PROVIDER_DATAGRAM_DISPATCH)impl->wsk_socket->Dispatch)
                ->WskBind(impl->wsk_socket, pSockAddr, 0, irp);
            break;
            
        default:
            IoFreeIrp(irp);
            return NET_ERROR_INVALID_PARAM;
    }
    
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }
    
    if (NT_SUCCESS(status)) 
        sock->addr = *addr;
    
    IoFreeIrp(irp);
    
    return NT_SUCCESS(status) ? NET_SUCCESS : convert_status_from_windows(status);
}

// Данная функция требует тестов!!!!
net_error_t windows_net_socket_connect(net_socket_t *sock, const net_address_t *addr) {
  if (!sock || !addr)
    return NET_ERROR_INVALID_PARAM;

  if (sock->type != NET_SOCK_TYPE_TCP_CONNECTION ||
      sock->protocol != NET_PROTO_TCP)
    return NET_ERROR_INVALID_PROTOCOL;

  if (sock->addr.family != addr->family)
    return NET_ERROR_INVALID_PARAM;

  PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
  if (!impl || !impl->wsk_socket)
    return NET_ERROR_INVALID_STATE;

  if (!impl->wsk_socket->Dispatch)
    return NET_ERROR_INVALID_VTABLE;

  NTSTATUS status;

  // WSK требует bind перед connect. Если сокет ещё не привязан — привязываем к
  // ANY:0 Определяем "не привязан" по нулевому family в сохранённом адресе
  // (после bind выше в windows_net_socket_bind sock->addr копируется из
  // пользовательского addr)
  BOOLEAN need_bind = FALSE;
  if (sock->addr.family == NET_AF_INET4) {
    if (sock->addr.addr.ipv4 == 0 && sock->addr.port == 0)
      need_bind = TRUE;
  } else if (sock->addr.family == NET_AF_INET6) {
    // проверяем, что ipv6 адрес весь нулевой и порт 0
    BOOLEAN all_zero = TRUE;
    for (int i = 0; i < 16; ++i) {
      if (sock->addr.addr.ipv6[i] != 0) {
        all_zero = FALSE;
        break;
      }
    }
    if (all_zero && sock->addr.port == 0)
      need_bind = TRUE;
  }

  if (need_bind) {
    net_address_t any_addr;
    RtlZeroMemory(&any_addr, sizeof(any_addr));
    any_addr.family = sock->addr.family;
    any_addr.port = 0;
    // addr внутри уже занулен RtlZeroMemory
    net_error_t be = windows_net_socket_bind(sock, &any_addr);
    if (be != NET_SUCCESS)
      return be;
  }

  // Готовим удалённый адрес
  SOCKADDR_STORAGE remote_storage;
  RtlZeroMemory(&remote_storage, sizeof(remote_storage));
  PSOCKADDR pRemote = NULL;

  if (addr->family == NET_AF_INET4) {
    PSOCKADDR_IN p4 = (PSOCKADDR_IN)&remote_storage;
    p4->sin_family = AF_INET;
    p4->sin_port = addr->port;
    p4->sin_addr.s_addr = addr->addr.ipv4;
    pRemote = (PSOCKADDR)p4;
  } else if (addr->family == NET_AF_INET6) {
    PSOCKADDR_IN6 p6 = (PSOCKADDR_IN6)&remote_storage;
    p6->sin6_family = AF_INET6;
    p6->sin6_port = addr->port;
    RtlCopyMemory(&p6->sin6_addr, addr->addr.ipv6, 16);
    p6->sin6_flowinfo = 0;
    p6->sin6_scope_id = 0;
    pRemote = (PSOCKADDR)p6;
  } else {
    return NET_ERROR_INVALID_PARAM;
  }

  PIRP irp = IoAllocateIrp(1, FALSE);
  if (!irp)
    return NET_ERROR_NO_MEMORY;

  KEVENT event;
  KeInitializeEvent(&event, NotificationEvent, FALSE);
  IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);

  status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)impl->wsk_socket->Dispatch)
               ->WskConnect(impl->wsk_socket, pRemote, 0, irp);

  if (status == STATUS_PENDING) {
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    status = irp->IoStatus.Status;
  }

  IoFreeIrp(irp);

  if (!NT_SUCCESS(status))
    return convert_status_from_windows(status);

  // После успешного connect — этот сокет активный для send/receive
  impl->active_client = impl->wsk_socket;

  sock->addr = *addr;

  return NET_SUCCESS;
}

// Данная функция требует тестов!!!!
net_error_t windows_net_socket_send(net_socket_t *sock, const void *data,
                                    size_t size, size_t *sent) {
  if (!sock || !data || size == 0)
    return NET_ERROR_INVALID_PARAM;

  if (sock->protocol != NET_PROTO_TCP)
    return NET_ERROR_INVALID_PROTOCOL;

  PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
  if (!impl)
    return NET_ERROR_INVALID_STATE;

  // Для TCP работаем через active_client: он установлен либо в accept
  // (серверная сторона), либо в connect (клиентская сторона)
  PWSK_SOCKET target = impl->active_client;
  if (!target || !target->Dispatch)
    return NET_ERROR_INVALID_STATE;

  NTSTATUS status;

  WSK_BUF wsk_buf;
  
  wsk_buf.Mdl = IoAllocateMdl((PVOID)data, (ULONG)size, FALSE, FALSE, NULL);
  if (!wsk_buf.Mdl)
    return NET_ERROR_NO_MEMORY;

  MmBuildMdlForNonPagedPool(wsk_buf.Mdl);

  wsk_buf.Offset = 0;
  wsk_buf.Length = size;

  PIRP irp = IoAllocateIrp(1, FALSE);
  if (!irp) {
    IoFreeMdl(wsk_buf.Mdl);
    return NET_ERROR_NO_MEMORY;
  }

  KEVENT event;
  KeInitializeEvent(&event, NotificationEvent, FALSE);
  IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);

  status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)target->Dispatch)
               ->WskSend(target, &wsk_buf,
                         0, // Flags — 0 для обычной отправки
                         irp);

  if (status == STATUS_PENDING) {
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    status = irp->IoStatus.Status;
  }

  SIZE_T bytes_sent = irp->IoStatus.Information;

  IoFreeIrp(irp);
  IoFreeMdl(wsk_buf.Mdl);

  if (!NT_SUCCESS(status))
    return convert_status_from_windows(status);

  if (sent)
    *sent = bytes_sent;

  return NET_SUCCESS;
}

net_error_t windows_net_socket_accept(net_socket_t* server, net_socket_t** client_out) {
    // Проверка параметров
    if (!server || !client_out) 
        return NET_ERROR_INVALID_PARAM;
    
    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)server->context;
    if (!impl) 
        return NET_ERROR_INVALID_STATE;
    
    // Проверка типа сокета
    if (server->type != NET_SOCK_TYPE_TCP_LISTEN) 
        return NET_ERROR_INVALID_PROTOCOL;
    
    if (!impl->wsk_socket) 
        return NET_ERROR_INVALID_STATE;
    
    if (!impl->wsk_socket->Dispatch) 
        return NET_ERROR_INVALID_VTABLE;
    
    NTSTATUS status;
    KEVENT event;
    PIRP irp;
    PWSK_SOCKET newClient = NULL;
    
    // Выделяем память под клиентский сокет
    net_socket_t* client = (net_socket_t*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(net_socket_t), 'kcoS');
    if (!client) 
        return NET_ERROR_NO_MEMORY;
    
    RtlZeroMemory(client, sizeof(net_socket_t));
    
    irp = IoAllocateIrp(1, FALSE);
    if (!irp) {
        ExFreePool(client);
        return NET_ERROR_NO_MEMORY;
    }
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);
    
    status = ((PWSK_PROVIDER_LISTEN_DISPATCH)impl->wsk_socket->Dispatch)
                ->WskAccept(
                    impl->wsk_socket,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    irp
                );
    
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }
    
    if (NT_SUCCESS(status)) {
        newClient = (PWSK_SOCKET)irp->IoStatus.Information;
    }
    
    IoFreeIrp(irp);
    
    if (!NT_SUCCESS(status) || !newClient) {
        ExFreePool(client);
        return convert_status_from_windows(status);
    }
    
   // Получаем адрес клиента
    SOCKADDR_STORAGE remote_addr;
    RtlZeroMemory(&remote_addr, sizeof(remote_addr));

    PIRP irp_addr = IoAllocateIrp(1, FALSE);
    if (irp_addr) {
        KEVENT event_addr;
        KeInitializeEvent(&event_addr, NotificationEvent, FALSE);
        IoSetCompletionRoutine(irp_addr, wsk_completion, &event_addr, TRUE, TRUE, TRUE);
        
        NTSTATUS addr_status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)newClient->Dispatch)
                                    ->WskGetRemoteAddress(
                                        newClient,
                                        (PSOCKADDR)&remote_addr,
                                        irp_addr
                                    );
        
        if (addr_status == STATUS_PENDING) {
            KeWaitForSingleObject(&event_addr, Executive, KernelMode, FALSE, NULL);
            addr_status = irp_addr->IoStatus.Status;
        }
        
        IoFreeIrp(irp_addr);
        
        if (NT_SUCCESS(addr_status)) {
            if (remote_addr.ss_family == AF_INET) {
                // IPv4
                PSOCKADDR_IN ipv4 = (PSOCKADDR_IN)&remote_addr;
                client->addr.family = NET_AF_INET4;
                client->addr.port = ipv4->sin_port;
                client->addr.addr.ipv4 = ipv4->sin_addr.s_addr;
            } 
            else if (remote_addr.ss_family == AF_INET6) {
                // IPv6
                PSOCKADDR_IN6 ipv6 = (PSOCKADDR_IN6)&remote_addr;
                client->addr.family = NET_AF_INET6;
                client->addr.port = ipv6->sin6_port;
                RtlCopyMemory(client->addr.addr.ipv6, &ipv6->sin6_addr, 16);
            }
        } else {
            // Заполняем нулями
            client->addr.family = NET_AF_INET4;
            client->addr.addr.ipv4 = 0;
            client->addr.port = 0;
        }
    }

    // Создаём impl для клиента
    PWINDOWS_SOCKET_IMPL client_impl = (PWINDOWS_SOCKET_IMPL)ExAllocatePool2(
        POOL_FLAG_NON_PAGED, sizeof(WINDOWS_SOCKET_IMPL), 'cskW');
    
    if (!client_impl) {
        ((PWSK_PROVIDER_BASIC_DISPATCH)newClient->Dispatch)->WskCloseSocket(newClient, NULL);
        ExFreePool(client);
        return NET_ERROR_NO_MEMORY;
    }
    
    RtlZeroMemory(client_impl, sizeof(WINDOWS_SOCKET_IMPL));
    client_impl->active_client = newClient;
    client_impl->wsk_socket = newClient;
    
    client->context = client_impl;
    client->protocol = NET_PROTO_TCP;
    client->type = NET_SOCK_TYPE_TCP_CONNECTION;
    
    *client_out = client;
    
    return NET_SUCCESS;
}

net_error_t windows_net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received) {
    if (!sock || !buffer || buffer_size == 0)
        return NET_ERROR_INVALID_PARAM;

    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl)
        return NET_ERROR_INVALID_STATE;

    NTSTATUS status;
    SIZE_T bytes_received = 0;

    WSK_BUF wsk_buf;
    wsk_buf.Mdl = IoAllocateMdl(buffer, (ULONG)buffer_size, FALSE, FALSE, NULL);

    if (!wsk_buf.Mdl)
        return NET_ERROR_NO_MEMORY;

    MmBuildMdlForNonPagedPool(wsk_buf.Mdl);

    wsk_buf.Offset = 0;
    wsk_buf.Length = buffer_size;

    KEVENT event;
    PIRP irp = IoAllocateIrp(1, FALSE);

    if (!irp) {
        IoFreeMdl(wsk_buf.Mdl);
        return NET_ERROR_NO_MEMORY;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);

    // TCP
    if (sock->protocol == NET_PROTO_TCP) {

        if (!impl->active_client) {
            IoFreeMdl(wsk_buf.Mdl);
            IoFreeIrp(irp);
            return NET_ERROR_INVALID_STATE;
        }

        status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)
            impl->active_client->Dispatch)->WskReceive(
                impl->active_client,
                &wsk_buf,
                0,
                irp
            );

        if (from_addr) 
            *from_addr = sock->addr;  // копируем сохраненный адрес
    }
    // UDP
    else {
        SOCKADDR_INET addr;
        ULONG addr_len = sizeof(addr);
        RtlZeroMemory(&addr, sizeof(addr));

        status = ((PWSK_PROVIDER_DATAGRAM_DISPATCH)
            impl->wsk_socket->Dispatch)->WskReceiveFrom(
                impl->wsk_socket,
                &wsk_buf,
                0,
                (PSOCKADDR)&addr,
                &addr_len,
                NULL,
                NULL,
                irp
            );

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = irp->IoStatus.Status;
        }

        if (from_addr && NT_SUCCESS(status)) {
            if (addr.si_family == AF_INET) {
                from_addr->family = NET_AF_INET4;
                from_addr->port = addr.Ipv4.sin_port;
                from_addr->addr.ipv4 = addr.Ipv4.sin_addr.s_addr;
            } else if (addr.si_family == AF_INET6) {
                from_addr->family = NET_AF_INET6;
                from_addr->port = addr.Ipv6.sin6_port;
                RtlCopyMemory(from_addr->addr.ipv6, addr.Ipv6.sin6_addr.u.Byte, 16);
            }
        }
    }

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }

    bytes_received = irp->IoStatus.Information;

    IoFreeIrp(irp);
    IoFreeMdl(wsk_buf.Mdl);

    if (!NT_SUCCESS(status))
        return convert_status_from_windows(status);

    if (received)
        *received = bytes_received;

    return NET_SUCCESS;
}

net_error_t windows_net_socket_get_address(net_socket_t* sock, net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t windows_net_socket_get_type(net_socket_t* sock, net_socket_type_t* type) {
    sock = 0;
    type = 0;
    return 0;
}

net_error_t windows_net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol) {
    sock = 0;
    protocol = 0;
    return 0;
}

// Данная функция требует тестов!!!!
net_error_t windows_net_socket_last_error(net_socket_t *sock, net_error_t* error) {
    if (!sock)
        return NET_ERROR_INVALID_PARAM;
    
    error = &sock->error;

    return NET_SUCCESS;
}

// Данная функция требует тестов!!!!
net_error_t windows_net_socket_last_platform_error(net_socket_t* sock, const void** platform_error) {
    if (!sock)
        return NET_ERROR_INVALID_PARAM;

    platform_error = sock->last_error;

    return NET_SUCCESS;
}

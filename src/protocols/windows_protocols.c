#include "../common/windows/windows_common.h"

static NTSTATUS wsk_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceObject);

    PKEVENT event = (PKEVENT)Context;

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

net_error_t windows_net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_flags_t flags, net_socket_t** socketOut) {
    NTSTATUS status;
    PWSK_SOCKET wsk_socket;
    PWINDOWS_SOCKET_IMPL impl;
    net_socket_t* sock;
    PIRP irp;
    KEVENT event;
    
    if (!socketOut) return NET_ERROR_INVALID_PARAM;
    
    // Создаем сокет
    sock = (net_socket_t*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(net_socket_t), 'kcoS');
    if (!sock) return NET_ERROR_NO_MEMORY;
    
    RtlZeroMemory(sock, sizeof(net_socket_t));
    sock->protocol = protocol;
    
    // Определяем параметры WSK сокета
    USHORT wsk_family = (family == NET_AF_INET4) ? AF_INET : AF_INET6;
    USHORT wsk_type = (protocol == NET_PROTO_TCP) ? SOCK_STREAM : SOCK_DGRAM;
    ULONG wsk_protocol = (protocol == NET_PROTO_TCP) ? IPPROTO_TCP : IPPROTO_UDP;
    
    // Флаги WSK
    ULONG wsk_flags = 0;
    if (protocol == NET_PROTO_TCP && (flags & NET_SOCK_FLAG_LISTENING)) {
        wsk_flags |= WSK_FLAG_LISTEN_SOCKET;
    }
    
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
        return NET_ERROR_GENERIC;
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
    impl->is_listening = (flags & NET_SOCK_FLAG_LISTENING) ? TRUE : FALSE;
    KeInitializeEvent(&impl->completion_event, NotificationEvent, FALSE);
    
    sock->context = impl;
    
    // Устанавливаем состояние сокета
    if (protocol == NET_PROTO_UDP) {
        sock->state = SOCK_STATE_UDP;
    } else if (flags & NET_SOCK_FLAG_LISTENING) {
        sock->state = SOCK_STATE_LISTENING;
    } else {
        sock->state = SOCK_STATE_INIT;
    }
    
    *socketOut = sock;
    
    return NET_SUCCESS;
}

net_error_t windows_net_socket_close(net_socket_t* sock)
{
    if (!sock) return NET_ERROR_INVALID_PARAM;
    
    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl || !impl->wsk_socket) return NET_ERROR_INVALID_STATE;
    
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

net_error_t windows_net_socket_set_options(net_socket_t* sock, const net_socket_options_t* opts) {
    sock = 0;
    opts = 0;
    return 0;
}

net_error_t windows_net_socket_get_options(net_socket_t* sock, net_socket_options_t* opts) {
    sock = 0;
    opts = 0;
    return 0;
}

net_error_t windows_net_socket_bind(net_socket_t* sock, const net_address_t* addr)
{
    if (!sock || !addr) return NET_ERROR_INVALID_PARAM;
    
    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl || !impl->wsk_socket) return NET_ERROR_INVALID_STATE;
    
    SOCKADDR_IN local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = addr->port;
    local_addr.sin_addr.s_addr = addr->addr.ipv4;
    
    PIRP irp = IoAllocateIrp(1, FALSE);
    if (!irp) return NET_ERROR_NO_MEMORY;
    
    KEVENT event;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);
    
    NTSTATUS status = ((PWSK_PROVIDER_LISTEN_DISPATCH)impl->wsk_socket->Dispatch)
                        ->WskBind(impl->wsk_socket, (PSOCKADDR)&local_addr, 0, irp);
    
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }
    
    IoFreeIrp(irp);
    
    return NT_SUCCESS(status) ? NET_SUCCESS : NET_ERROR_GENERIC;
}

net_error_t windows_net_socket_connect(net_socket_t* sock, const net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t windows_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent) {
    sock = 0;
    data = 0;
    size = 0;
    sent = 0;
    return 0;
}

net_error_t windows_net_socket_accept(net_socket_t* server, net_socket_t** client_out)
{
    PWINDOWS_SOCKET_IMPL impl = (PWINDOWS_SOCKET_IMPL)server->context;
    if (!impl) return NET_ERROR_INVALID_STATE;
    if (!impl->is_listening) return NET_ERROR_INVALID_STATE;
    if (!impl->wsk_socket) return NET_ERROR_INVALID_STATE;
    if (!impl->wsk_socket->Dispatch) return NET_ERROR_INVALID_VTABLE;
    
    NTSTATUS status;
    KEVENT event;
    PIRP irp;
    PWSK_SOCKET newClient = NULL;

    // Выделяем память под клиентский сокет
    net_socket_t* client = (net_socket_t*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(net_socket_t), 'kcoS');
    if (!client) return NET_ERROR_NO_MEMORY;
    
    RtlZeroMemory(client, sizeof(net_socket_t));
    
    irp = IoAllocateIrp(1, FALSE);
    if (!irp) {
        DbgPrint("[ACCEPT] No IRP\n");
        return NET_ERROR_NO_MEMORY;
    }
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(irp, wsk_completion, &event, TRUE, TRUE, TRUE);
    
    // Вызов WskAccept
    status = ((PWSK_PROVIDER_LISTEN_DISPATCH)impl->wsk_socket->Dispatch)
                ->WskAccept(
                    impl->wsk_socket,      // ListenSocket
                    0,                     // Flags (зарезервирован, всегда 0)
                    NULL,                  // AcceptSocketContext
                    NULL,                  // AcceptSocketDispatch
                    NULL,                  // LocalAddress (можно NULL)
                    NULL,                  // RemoteAddress (можно NULL)
                    irp                    // Irp
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
        ExFreePool(client); // освобождаем клиентский сокет
        return convert_status_from_windows(status);
    }
    
    // Создаём impl для клиента
    PWINDOWS_SOCKET_IMPL client_impl = (PWINDOWS_SOCKET_IMPL)ExAllocatePool2(
        POOL_FLAG_NON_PAGED, sizeof(WINDOWS_SOCKET_IMPL), 'cskW');
    
    if (!client_impl) {
        ((PWSK_PROVIDER_BASIC_DISPATCH)newClient->Dispatch)->WskCloseSocket(newClient, NULL);
        return NET_ERROR_NO_MEMORY;
    }
    
    RtlZeroMemory(client_impl, sizeof(WINDOWS_SOCKET_IMPL));
    client_impl->active_client = newClient;
    client_impl->wsk_socket = newClient;
    
    client->context = client_impl;
    client->protocol = NET_PROTO_TCP;
    client->state = SOCK_STATE_CONNECTED;
    
    *client_out = client;

    return NET_SUCCESS;
}

net_error_t windows_net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received)
{
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
    }
    // UDP
    else {

        SOCKADDR_INET addr;
        ULONG addr_len = sizeof(addr);

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

        if (from_addr && NT_SUCCESS(status)) {
            // можно дополнить разбор адреса
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


net_error_t windows_net_socket_get_local_address(net_socket_t* sock, net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t windows_net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t windows_net_socket_set_nonblocking(net_socket_t* sock, int enable) {
    sock = 0;
    enable = 0;
    return 0;
}

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

#include "../common/windows/windows_common.h"

net_error_t windows_net_socket_create(net_family_t family, net_protocol_t protocol, int flags, net_socket_t* socket_out) {
    family = 0;
    protocol = 0;
    flags = 0;
    socket_out = 0;
    return 0;
}

net_error_t windows_net_socket_close(net_socket_t* sock) {
    sock = 0;
    return 0;
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

net_error_t windows_net_socket_bind(net_socket_t* sock, const net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
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


static NTSTATUS wsk_completion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
) {
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceObject);

    PKEVENT event = (PKEVENT)Context;

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS wsk_receive_internal(
    PWSK_SOCKET socket,
    PWSK_BUF wsk_buf,
    SIZE_T* bytes_received,
    PSOCKADDR remote_addr,      // NULL для TCP
    PULONG remote_addr_len      // NULL для TCP
) {
    NTSTATUS status;
    KEVENT event;
    PIRP irp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoAllocateIrp(1, FALSE);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(
        irp,
        wsk_completion,
        &event,
        TRUE, TRUE, TRUE
    );

    if (remote_addr) {
        // UDP
        status = ((PWSK_PROVIDER_DATAGRAM_DISPATCH)socket->Dispatch)->
            WskReceiveFrom(
                socket,
                wsk_buf,
                0,
                remote_addr,
                remote_addr_len,
                NULL,
                NULL,
                irp
            );
    } else {
        // TCP
        status = ((PWSK_PROVIDER_CONNECTION_DISPATCH)socket->Dispatch)->
            WskReceive(
                socket,
                wsk_buf,
                0,
                irp
            );
    }

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }

    if (bytes_received) {
        *bytes_received = irp->IoStatus.Information;
    }

    IoFreeIrp(irp);

    return status;
}

net_error_t windows_net_socket_receive(
    net_socket_t* sock,
    void* buffer,
    size_t buffer_size,
    net_address_t* from_addr,
    size_t* received
) {
    NTSTATUS status;
    SIZE_T bytes_received = 0;
    PWSK_BUF wsk_buf;
    PWINDOWS_SOCKET_IMPL impl;

    if (!sock || !buffer || buffer_size == 0) {
        return NET_ERROR_INVALID_PARAM;
    }

    if (received) *received = 0;
    if (from_addr) RtlZeroMemory(from_addr, sizeof(net_address_t));

    impl = (PWINDOWS_SOCKET_IMPL)sock->context;
    if (!impl || !impl->wsk_socket) {
        return NET_ERROR_INVALID_STATE;
    }

    // ===== подготовка WSK_BUF =====
    wsk_buf = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(WSK_BUF), 'buSW');
    if (!wsk_buf) return NET_ERROR_NO_MEMORY;

    wsk_buf->Mdl = IoAllocateMdl(buffer, (ULONG)buffer_size, FALSE, FALSE, NULL);
    if (!wsk_buf->Mdl) {
        ExFreePool(wsk_buf);
        return NET_ERROR_NO_MEMORY;
    }

    MmBuildMdlForNonPagedPool(wsk_buf->Mdl);
    wsk_buf->Offset = 0;
    wsk_buf->Length = buffer_size;

    // ===== TCP =====
    if (sock->protocol == NET_PROTO_TCP) {

        // ❗ accept делается отдельно, здесь предполагаем что active_client уже есть
        if (!impl->active_client) {
            IoFreeMdl(wsk_buf->Mdl);
            ExFreePool(wsk_buf);
            return NET_ERROR_INVALID_STATE;
        }

        status = wsk_receive_internal(
            impl->active_client,
            wsk_buf,
            &bytes_received,
            NULL,
            NULL
        );
    }

    // ===== UDP =====
    else if (sock->protocol == NET_PROTO_UDP) {

        SOCKADDR_INET addr;
        ULONG addr_len = sizeof(addr);

        status = wsk_receive_internal(
            impl->wsk_socket,
            wsk_buf,
            &bytes_received,
            (PSOCKADDR)&addr,
            &addr_len
        );

        if (NT_SUCCESS(status) && from_addr) {
            if (addr.si_family == AF_INET) {
                from_addr->family = NET_AF_INET4;
                from_addr->port = addr.Ipv4.sin_port;
                from_addr->addr.ipv4 = addr.Ipv4.sin_addr.s_addr;
            } else {
                from_addr->family = NET_AF_INET6;
                from_addr->port = addr.Ipv6.sin6_port;
                RtlCopyMemory(from_addr->addr.ipv6, addr.Ipv6.sin6_addr.u.Byte, 16);
            }
        }
    }
    else {
        IoFreeMdl(wsk_buf->Mdl);
        ExFreePool(wsk_buf);
        return NET_ERROR_INVALID_PROTOCOL;
    }

    IoFreeMdl(wsk_buf->Mdl);
    ExFreePool(wsk_buf);

    if (!NT_SUCCESS(status)) {
        return convert_status_from_windows(status);
    }

    if (received) {
        *received = bytes_received;
    }

    return NET_SUCCESS;
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

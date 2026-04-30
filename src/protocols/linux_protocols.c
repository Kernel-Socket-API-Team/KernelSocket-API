#include "../common/linux/linux_common.h"

net_error_t linux_net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type,
                                    net_socket_t** socketOut)
{

    /* TODO (требуется написать linux_net_is_ready):
    net_error_t lib_state = linux_net_is_ready();
    if (lib_state != NET_SUCCESS)
        return lib_state;
    */

    if (!socketOut)
        return NET_ERROR_INVALID_PARAM;

    if (protocol == NET_PROTO_UDP && type != NET_SOCK_TYPE_UDP)
        return NET_ERROR_INVALID_PARAM;

    if (protocol == NET_PROTO_TCP && type == NET_SOCK_TYPE_UDP)
        return NET_ERROR_INVALID_PARAM;

    if (type != NET_SOCK_TYPE_TCP_LISTEN && type != NET_SOCK_TYPE_TCP_CONNECTION && type != NET_SOCK_TYPE_UDP)
        return NET_ERROR_INVALID_PARAM;

    int status;
    struct socket* kernel_socket;
    PLINUX_SOCKET_IMPL impl;
    net_socket_t* sock;

    // Переводим наши типы в типы, понятные ядру
    int af = (family == NET_AF_INET4) ? AF_INET : AF_INET6;
    int kind = (protocol == NET_PROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int proto = (protocol == NET_PROTO_UDP) ? IPPROTO_UDP : IPPROTO_TCP;

    // Создаём сокет ядра
    kernel_socket = NULL;
    status = sock_create_kern(&init_net, af, kind, proto, &kernel_socket);
    if (status < 0)
        return convert_status_from_linux(status);

    // Выделяем память
    sock = kmalloc(sizeof(net_socket_t), GFP_KERNEL);
    if (!sock)
    {
        sock_release(kernel_socket);
        return NET_ERROR_NO_MEMORY;
    }
    memset(sock, 0, sizeof(net_socket_t));

    // Создаём Linux контекст
    impl = kmalloc(sizeof(LINUX_SOCKET_IMPL), GFP_KERNEL);
    if (!impl)
    {
        sock_release(kernel_socket);
        kfree(sock);
        return NET_ERROR_NO_MEMORY;
    }
    memset(impl, 0, sizeof(LINUX_SOCKET_IMPL));

    impl->kernel_socket = kernel_socket;

    sock->addr.family = family;
    sock->context = impl;
    sock->protocol = protocol;
    sock->type = type;

    *socketOut = sock;

    return NET_SUCCESS;
}

net_error_t linux_net_socket_close(net_socket_t* sock)
{
    if (!sock)
        return NET_ERROR_INVALID_PARAM;

    PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
    if (!impl || !impl->kernel_socket)
        return NET_ERROR_INVALID_STATE;

    // Закрываем клиентский сокет (если есть)
    if (impl->active_client && impl->active_client != impl->kernel_socket)
    {
        sock_release(impl->active_client);
        impl->active_client = NULL;
    }

    // Закрываем основной системный сокет
    if (impl->kernel_socket)
    {
        sock_release(impl->kernel_socket);
        impl->kernel_socket = NULL;
    }

    // Освобождаем память
    kfree(impl);
    sock->context = NULL;

    kfree(sock);

    return NET_SUCCESS;
}

net_error_t linux_net_socket_bind(net_socket_t* sock, const net_address_t* addr)
{

    if (!sock || !addr)
        return NET_ERROR_INVALID_PARAM;

    if (sock->addr.family != addr->family)
        return NET_ERROR_INVALID_PARAM;

    PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
    if (!impl || !impl->kernel_socket)
        return NET_ERROR_INVALID_STATE;

    int status;

    // Заполняем структуру адреса ядра
    if (addr->family == NET_AF_INET4)
    {

        struct sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));

        local_addr.sin_family = AF_INET;
        local_addr.sin_port = addr->port;
        local_addr.sin_addr.s_addr = addr->addr.ipv4;

        // bind
        status = kernel_bind(impl->kernel_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));
    }
    else if (addr->family == NET_AF_INET6)
    {

        struct sockaddr_in6 local_addr;
        memset(&local_addr, 0, sizeof(local_addr));

        local_addr.sin6_family = AF_INET6;
        local_addr.sin6_port = addr->port;
        local_addr.sin6_scope_id = addr->scope_id;
        memcpy(&local_addr.sin6_addr, addr->addr.ipv6, 16);

        status = kernel_bind(impl->kernel_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));
    }
    else
    {
        return NET_ERROR_INVALID_PARAM;
    }

    if (status < 0)
    {
        return convert_status_from_linux(status);
    }

    sock->addr = *addr;

    return NET_SUCCESS;
}

net_error_t linux_net_socket_connect(net_socket_t* sock, const net_address_t* addr)
{
    if (!sock || !addr)
        return NET_ERROR_INVALID_PARAM;

    PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
    if (!impl || !impl->kernel_socket)
        return NET_ERROR_INVALID_STATE;

    int status;

    if (addr->family == NET_AF_INET4)
    {
        struct sockaddr_in remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = addr->port;
        remote_addr.sin_addr.s_addr = addr->addr.ipv4;

        status = kernel_connect(impl->kernel_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr), 0);
    }
    else if (addr->family == NET_AF_INET6)
    {
        struct sockaddr_in6 remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin6_family = AF_INET6;
        remote_addr.sin6_port = addr->port;
        remote_addr.sin6_scope_id = addr->scope_id;
        memcpy(&remote_addr.sin6_addr, addr->addr.ipv6, 16);

        status = kernel_connect(impl->kernel_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr), 0);
    }
    else
    {
        return NET_ERROR_INVALID_PARAM;
    }

    if (status < 0)
    {
        return convert_status_from_linux(status);
    }

    sock->remote_addr = *addr;

    return NET_SUCCESS;
}

net_error_t linux_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent)
{

    if (!sock || !data || size == 0)
        return NET_ERROR_INVALID_PARAM;

    PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
    if (!impl)
        return NET_ERROR_INVALID_STATE;

    // Выбираем правильный сокет
    struct socket* target = NULL;

    if (sock->type == NET_SOCK_TYPE_TCP_LISTEN)
    {
        // TCP сервер — шлём через active_client
        if (!impl->active_client)
            return NET_ERROR_INVALID_STATE;
        target = impl->active_client;
    }
    else
    {
        // TCP клиент или UDP — шлём через основной сокет
        if (!impl->kernel_socket)
            return NET_ERROR_INVALID_STATE;
        target = impl->kernel_socket;
    }

    // Готовим данные для отправки
    struct kvec kv;
    kv.iov_base = (void*)data;
    kv.iov_len = size;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    // Для UDP кладём адрес получателя
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;

    if (sock->protocol == NET_PROTO_UDP)
    {

        // Проверяем что remote_addr заполнен
        if (sock->remote_addr.port == 0)
            return NET_ERROR_INVALID_STATE;

        if (sock->remote_addr.family == NET_AF_INET4)
        {

            memset(&addr4, 0, sizeof(addr4));
            addr4.sin_family = AF_INET;
            addr4.sin_port = sock->remote_addr.port;
            addr4.sin_addr.s_addr = sock->remote_addr.addr.ipv4;

            msg.msg_name = &addr4;
            msg.msg_namelen = sizeof(addr4);
        }
        else
        {

            memset(&addr6, 0, sizeof(addr6));
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = sock->remote_addr.port;
            addr6.sin6_scope_id = sock->remote_addr.scope_id;
            memcpy(&addr6.sin6_addr, sock->remote_addr.addr.ipv6, 16);

            msg.msg_name = &addr6;
            msg.msg_namelen = sizeof(addr6);
        }
    }
    // TCP — msg_name не нужен, соединение уже установлено

    // Отправляем
    int status = kernel_sendmsg(target, &msg, &kv, 1, size);
    if (status < 0)
        return convert_status_from_linux(status);

    // Сколько байт отправили
    if (sent)
        *sent = (size_t)status;

    return NET_SUCCESS;
}

net_error_t linux_net_socket_accept(net_socket_t* server, net_socket_t** client_out)
{
    server = 0;
    client_out = 0;
    return 0;
}

net_error_t linux_net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr,
                                     size_t* received)
{
    sock = 0;
    buffer = 0;
    buffer_size = 0;
    from_addr = 0;
    received = 0;
    return 0;
}

net_error_t linux_net_socket_get_address(net_socket_t* sock, net_address_t* addr)
{
    if (!sock || !addr)
        return NET_ERROR_INVALID_PARAM;

    *addr = sock->addr;
    return NET_SUCCESS;
}

net_error_t linux_net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr)
{
    if (!sock || !addr)
        return NET_ERROR_INVALID_PARAM;

    *addr = sock->remote_addr;
    return NET_SUCCESS;
}

net_error_t linux_net_socket_get_type(net_socket_t* sock, net_socket_type_t* type)
{
    if (!sock || !type)
        return NET_ERROR_INVALID_PARAM;

    *type = sock->type;
    return NET_SUCCESS;
}

net_error_t linux_net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol)
{
    if (!sock || !protocol)
        return NET_ERROR_INVALID_PARAM;

    *protocol = sock->protocol;
    return NET_SUCCESS;
}

net_error_t linux_net_socket_last_error(net_socket_t* sock, net_error_t* error)
{
    if (!sock || !error)
        return NET_ERROR_INVALID_PARAM;

    *error = sock->error;
    return NET_SUCCESS;
}

net_error_t linux_net_socket_last_platform_error(net_socket_t* sock, const void** platform_error)
{
    if (!sock || !platform_error)
        return NET_ERROR_INVALID_PARAM;

    *platform_error = (const void*)sock->last_error;
    return NET_SUCCESS;
}
#include "../common/linux/linux_common.h"

net_error_t linux_net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type, net_socket_t** socketOut) {

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

    if (type != NET_SOCK_TYPE_TCP_LISTEN &&
        type != NET_SOCK_TYPE_TCP_CONNECTION && type != NET_SOCK_TYPE_UDP)
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
    if (!sock) {
        sock_release(kernel_socket);
        return NET_ERROR_NO_MEMORY;
    }
    memset(sock, 0, sizeof(net_socket_t));

    // Создаём Linux контекст
    impl = kmalloc(sizeof(LINUX_SOCKET_IMPL), GFP_KERNEL);
    if (!impl) {
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


net_error_t linux_net_socket_close(net_socket_t *sock) {
  if (!sock)
    return NET_ERROR_INVALID_PARAM;

  PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
  if (!impl || !impl->kernel_socket)
    return NET_ERROR_INVALID_STATE;

  // Закрываем клиентский сокет (если есть)
  if (impl->active_client && impl->active_client != impl->kernel_socket) {
    sock_release(impl->active_client);
    impl->active_client = NULL;
  }

  // Закрываем основной системный сокет
  if (impl->kernel_socket) {
    sock_release(impl->kernel_socket);
    impl->kernel_socket = NULL;
  }

  // Освобождаем память
  kfree(impl);
  sock->context = NULL;

  kfree(sock);

  return NET_SUCCESS;
}

net_error_t linux_net_socket_bind(net_socket_t* sock, const net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t linux_net_socket_connect(net_socket_t* sock, const net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t linux_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent) {
    sock = 0;
    data = 0;
    size = 0;
    sent = 0;
    return 0;
}

net_error_t linux_net_socket_accept(net_socket_t* server, net_socket_t** client_out) {
    server = 0;
    client_out = 0;
    return 0;
}

net_error_t linux_net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received) {
    sock = 0;
    buffer = 0;
    buffer_size = 0;
    from_addr = 0;
    received = 0;
    return 0;
}

net_error_t linux_net_socket_get_address(net_socket_t* sock, net_address_t* addr) {
    sock = 0;
    addr = 0;
    return 0;
}

net_error_t linux_net_socket_get_type(net_socket_t* sock, net_socket_type_t* type) {
    sock = 0;
    type = 0;
    return 0;
}

net_error_t linux_net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol) {
    sock = 0;
    protocol = 0;
    return 0;
}

net_error_t linux_net_socket_last_error(net_socket_t *sock, net_error_t* error) {
    sock = 0;
    error = 0;
    return 0;
}

net_error_t linux_net_socket_last_platform_error(net_socket_t* sock, const void** platform_error) {
    sock = 0;
    platform_error = 0;
    return 0;
}
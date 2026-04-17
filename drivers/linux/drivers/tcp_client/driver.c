#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/in.h>
#include <linux/inet.h>
#include <linux/net.h>
#include <net/sock.h>

#include <linux/slab.h>   // kmalloc и kfree
#include <linux/string.h> // memset

// Драйвер будет дополняться функционалом, необходимым
// в linux_protocols.c
// Пока реализованы:
// linux_net_socket_create(...);
// linux_net_socket_close(...);
// linux_net_socket_bind(...);
// linux_net_socket_send(...);
// linux_net_socket_connect(...);


// Определяем необходимые для работы структуры и функции

typedef enum {

    /* Ошибки инициализации библиотеки */
    NET_ERROR_NOT_INITIALIZED = 0, // Библиотека не инициализирована
    NET_ERROR_NOT_REGISTER = -1,   // Библиотека не зарегистрирована
    NET_ERROR_INVALID_VTABLE = -2, // Некорректная виртуальная таблица
    NET_SUCCESS = -3,              // Успешное выполнение операции

    /* Платформенные ошибки */
    NET_ERROR_NO_MEMORY = -4,        // Недостаточно памяти
    NET_ERROR_ACCESS_DENIED = -5,    // Доступ запрещен
    NET_ERROR_TIMEOUT = -6,          // Превышено время ожидания
    NET_ERROR_BUFFER_TOO_SMALL = -7, // Размер буфера слишком мал
    NET_ERROR_INVALID_PARAM = -8,    // Некорректный параметр
    NET_ERROR_GENERIC = -9,          // Общая ошибка

    /* Ошибки работы с сокетами */
    NET_ERROR_INVALID_STATE = -10,   // Сокет в невалидном состоянии
    NET_ERROR_INVALID_PROTOCOL = -11 // Некорректный протокол

} net_error_t;

typedef enum {
    SOCK_STATE_INIT = 0,      // Только создан
    SOCK_STATE_BOUND = 1,     // Привязан к адресу
    SOCK_STATE_LISTENING = 2, // TCP в режиме прослушивания
    SOCK_STATE_CONNECTED = 3, // TCP подключен (клиент или принятый)
    SOCK_STATE_UDP = 4,       // UDP сокет
} net_socket_state_t;

/* Типы транспортных протоколов */
typedef enum {
    NET_PROTO_TCP = 1, // TCP протокол
    NET_PROTO_UDP = 2  // UDP протокол
} net_protocol_t;

/* Семейства адресов (типы IP) */
typedef enum {
    NET_AF_INET4 = 4, // IPv4
    NET_AF_INET6 = 6  // IPv6
} net_family_t;

/* Типы сокетов */
typedef enum {
    NET_SOCK_TYPE_TCP_LISTEN,     // Слушающий TCP сокет (сервер)
    NET_SOCK_TYPE_TCP_CONNECTION, // Подключенный TCP сокет (клиент)
    NET_SOCK_TYPE_UDP             // UDP сокет
} net_socket_type_t;

typedef struct net_address {
    net_family_t family; // Семейство адресов (IPv4/IPv6)
    uint16_t port;       // Порт (в сетевом порядке байт)
    union {
        uint32_t ipv4;    // IPv4 адрес (в сетевом порядке)
        uint8_t ipv6[16]; // IPv6 адрес (в сетевом порядке)
    } addr;
    char hostname[256]; // Человекочитаемое имя (опционально)
} net_address_t;

// Реализация сокета (скрыта от пользователя)
typedef struct net_socket {
    net_protocol_t protocol;    // Тип транспортного протокола (TCP/UDP)
    net_address_t addr;         // Настройки локального адреса сокета
    net_address_t remote_addr;  // Настройки удалённого адреса сокета

    net_error_t
        error; // Храним последнюю ошибку, которая возникла при работе с сокетом
    void* last_error; // Храним указатель на последнюю ошибку в контексте
    // конкретной ОС (NTSTATUS ...)

    net_socket_type_t type; // Тип сокета

    void* context; // Платформозависимый контекст
} net_socket_t;

typedef struct LINUX_SOCKET_IMPL {
    struct socket* kernel_socket; // kernel socket
    struct socket* active_client; // Для TCP клиента
} LINUX_SOCKET_IMPL, * PLINUX_SOCKET_IMPL;

net_error_t linux_net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type, net_socket_t** socketOut);
net_error_t linux_net_socket_close(net_socket_t* sock);
net_error_t linux_net_socket_bind(net_socket_t* sock, const net_address_t* addr);
net_error_t linux_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);
net_error_t linux_net_socket_connect(net_socket_t* sock, const net_address_t* addr);


// Конвертация ошибок
net_error_t convert_status_from_linux(int error);

// сюда принимаем выходной сокет
static net_socket_t* g_test_socket = NULL;

static int __init minimal_driver_init(void) {
    printk(KERN_INFO "[driver] Loading module...\n");



    net_socket_t* sock = NULL;

    // IP, protocol, sock_type
    net_error_t status = linux_net_socket_create(NET_AF_INET4,
        NET_PROTO_TCP,
        NET_SOCK_TYPE_TCP_CONNECTION,
        &sock);

    if (status != NET_SUCCESS) {
        printk(KERN_ERR "[driver] Failed to create socket, error code: %d\n",
            status);
        return -1;
    }

    net_address_t addr;
    memset(&addr, 0, sizeof(addr));
    addr.family = NET_AF_INET4;
    addr.port = 0;
    addr.addr.ipv4 = 0;

    // Привязываем
    status = linux_net_socket_bind(sock, &addr);
    if (status != NET_SUCCESS) {
        printk(KERN_ERR "[driver] bind failed: %d\n", status);
        linux_net_socket_close(sock);
        return -1;
    }
    printk(KERN_INFO "[driver] Bind OK\n");

    // Здесь необходим net_error_t linux_net_address_parse(const char* str, 
    //                                              net_family_t ip_family, 
    //                                              net_address_t* addr)
    // Вместо реализации ниже
    
    // Заполняем remote_addr — куда слать
    // htonl/htons нужны здесь потому что мы заполняем вручную
    sock->remote_addr.family = NET_AF_INET4;
    sock->remote_addr.port = htons(9002);
    in4_pton("192.168.203.1", -1, (u8 *)&sock->remote_addr.addr.ipv4, -1, NULL);

    // Отправляем
    char* msg = "Hello matha faka!";
    size_t sent = 0;

    status = linux_net_socket_connect(sock, &sock->remote_addr);
    if (status != NET_SUCCESS) {
        printk(KERN_ERR "[driver] Connect failed: %d\n", status);
        linux_net_socket_close(sock);
        return -1;
    }

    status = linux_net_socket_send(sock, msg, strlen(msg), &sent);
    if (status != NET_SUCCESS)
        printk(KERN_ERR "[driver] send failed: %d\n", status);
    else
        printk(KERN_INFO "[driver] sent %zu bytes\n", sent);

    g_test_socket = sock;
    return 0;
}

static void __exit minimal_driver_exit(void) {
    if (g_test_socket) {
        if (linux_net_socket_close(g_test_socket) != NET_SUCCESS) {
            printk(KERN_ERR "[driver] Socket %p was not closed if it exist\n", g_test_socket);
        }
        else
            printk(KERN_INFO "[driver] Socket was successfully closed\n");
    }
    printk(KERN_INFO "[driver] Module unloaded\n");
}

// Регистрируем функции загрузки и выгрузки
module_init(minimal_driver_init);
module_exit(minimal_driver_exit);

// Обязательная информация о модуле
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vyacheslav");
MODULE_DESCRIPTION("Minimal Linux Kernel Module");
MODULE_VERSION("1.0");

// Реализация функций

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

net_error_t linux_net_socket_close(net_socket_t* sock) {
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

    if (!sock || !addr)
        return NET_ERROR_INVALID_PARAM;

    if (sock->addr.family != addr->family)
        return NET_ERROR_INVALID_PARAM;

    PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
    if (!impl || !impl->kernel_socket)
        return NET_ERROR_INVALID_STATE;

    int status;

    // Заполняем структуру адреса ядра
    if (addr->family == NET_AF_INET4) {

        struct sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));

        local_addr.sin_family = AF_INET;
        local_addr.sin_port = addr->port;
        local_addr.sin_addr.s_addr = addr->addr.ipv4;

        // bind
        status = kernel_bind(impl->kernel_socket,
            (struct sockaddr*)&local_addr,
            sizeof(local_addr));

    }
    else if (addr->family == NET_AF_INET6) {

        struct sockaddr_in6 local_addr;
        memset(&local_addr, 0, sizeof(local_addr));

        local_addr.sin6_family = AF_INET6;
        local_addr.sin6_port = addr->port;
        memcpy(&local_addr.sin6_addr, addr->addr.ipv6, 16);

        status = kernel_bind(impl->kernel_socket,
            (struct sockaddr*)&local_addr,
            sizeof(local_addr));

    }
    else {
        return NET_ERROR_INVALID_PARAM;
    }

    if (status < 0) {
        return convert_status_from_linux(status);
    }

    sock->addr = *addr;

    return NET_SUCCESS;
}

net_error_t linux_net_socket_connect(net_socket_t* sock, const net_address_t* addr) {
    if (!sock || !addr)
        return NET_ERROR_INVALID_PARAM;

    PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
    if (!impl || !impl->kernel_socket)
        return NET_ERROR_INVALID_STATE;

    int status;

    if (addr->family == NET_AF_INET4) {
        struct sockaddr_in remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = addr->port;
        remote_addr.sin_addr.s_addr = addr->addr.ipv4;

        status = kernel_connect(impl->kernel_socket,
            (struct sockaddr*)&remote_addr,
            sizeof(remote_addr), 0);
    }
    else if (addr->family == NET_AF_INET6) {
        struct sockaddr_in6 remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin6_family = AF_INET6;
        remote_addr.sin6_port = addr->port;
        memcpy(&remote_addr.sin6_addr, addr->addr.ipv6, 16);

        status = kernel_connect(impl->kernel_socket,
            (struct sockaddr*)&remote_addr,
            sizeof(remote_addr), 0);
    }
    else {
        return NET_ERROR_INVALID_PARAM;
    }

    if (status < 0) {
        printk(KERN_ERR "[driver] Connect failed with status: %d\n", status);
        return convert_status_from_linux(status);
    }

    sock->remote_addr = *addr;

    return NET_SUCCESS;
}

net_error_t linux_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent) {

    if (!sock || !data || size == 0)
        return NET_ERROR_INVALID_PARAM;

    PLINUX_SOCKET_IMPL impl = (PLINUX_SOCKET_IMPL)sock->context;
    if (!impl)
        return NET_ERROR_INVALID_STATE;

    // Выбираем правильный сокет
    struct socket* target = NULL;

    if (sock->type == NET_SOCK_TYPE_TCP_LISTEN) {
        // TCP сервер — шлём через active_client
        if (!impl->active_client)
            return NET_ERROR_INVALID_STATE;
        target = impl->active_client;

    }
    else {
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
    struct sockaddr_in  addr4;
    struct sockaddr_in6 addr6;

    if (sock->protocol == NET_PROTO_UDP) {

        // Проверяем что remote_addr заполнен
        if (sock->remote_addr.port == 0)
            return NET_ERROR_INVALID_STATE;

        if (sock->remote_addr.family == NET_AF_INET4) {

            memset(&addr4, 0, sizeof(addr4));
            addr4.sin_family = AF_INET;
            addr4.sin_port = sock->remote_addr.port;
            addr4.sin_addr.s_addr = sock->remote_addr.addr.ipv4;

            msg.msg_name = &addr4;
            msg.msg_namelen = sizeof(addr4);

        }
        else {

            memset(&addr6, 0, sizeof(addr6));
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = sock->remote_addr.port;
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

net_error_t convert_status_from_linux(int error) {

    if (error >= 0) {
        return NET_SUCCESS;
    }

    int code = -error;

    switch (code) {
    case ENOMEM:        return NET_ERROR_NO_MEMORY;
    case EACCES:        return NET_ERROR_ACCESS_DENIED;
    case EPERM:         return NET_ERROR_ACCESS_DENIED;
    case EINVAL:        return NET_ERROR_INVALID_PARAM;
    case ETIMEDOUT:     return NET_ERROR_TIMEOUT;
    case EMSGSIZE:      return NET_ERROR_BUFFER_TOO_SMALL;
    case ENOBUFS:       return NET_ERROR_NO_MEMORY;

        // Специфические ошибки
    case ECONNREFUSED:
    case EADDRINUSE:
    case ENETUNREACH:
    case EAGAIN:        return NET_ERROR_GENERIC;

    default:            return NET_ERROR_GENERIC;
    }
}
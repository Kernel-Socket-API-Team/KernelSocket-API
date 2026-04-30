#include "../../../../include/ksockapi.h"
// второстпенное
#include <linux/inet.h>   // in4_pton
#include <linux/string.h> // memset

// сюда принимаем выходной сокет
static net_socket_t* g_test_socket = NULL;

int debug_port = 9001; // TCP: 9001 - IPv4
                       //      9002 - IPv6
                       // UDP: 9003 - IPv4
                       //      9004 - IPv6

static int __init minimal_driver_init(void)
{

    // Чтобы удобно переключаться ----------------------------
    net_family_t debug_family;
    net_protocol_t debug_proto;
    net_socket_type_t debug_sock_type;

    switch (debug_port)
    {
    case 9001:
    {
        debug_family = NET_AF_INET4;
        debug_proto = NET_PROTO_TCP;
        debug_sock_type = NET_SOCK_TYPE_TCP_CONNECTION;
        break;
    }
    case 9002:
    {
        debug_family = NET_AF_INET6;
        debug_proto = NET_PROTO_TCP;
        debug_sock_type = NET_SOCK_TYPE_TCP_CONNECTION;
        break;
    }
    case 9003:
    {
        debug_family = NET_AF_INET4;
        debug_proto = NET_PROTO_UDP;
        debug_sock_type = NET_SOCK_TYPE_UDP;
        break;
    }
    case 9004:
    {
        debug_family = NET_AF_INET6;
        debug_proto = NET_PROTO_UDP;
        debug_sock_type = NET_SOCK_TYPE_UDP;
        break;
    }
    }
    // -------------------------------------------------------

    printk(KERN_INFO "[driver] Loading module...\n");

    net_socket_t* sock = NULL;

    // IP, protocol, sock_type
    net_error_t status = net_socket_create(debug_family, debug_proto, debug_sock_type, &sock);

    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] Failed to create socket, error code: %d\n", status);
        return -1;
    }

    net_address_t local_addr; // локальный адрес
    // TODO(begin): net_address_parse("{ip сервера (как v4, так и v6)}", debug_family, &local_addr);
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.family = debug_family;

    // scope_id для bind не 0 тогда и только тогда, когда нужно привязать
    // к конкретному интерфейсу: "Эй, я хочу принимать данные с этого интерфейса"
    // Для клиента он НЕ нужен, не используется.
    // Для сервера можно указать конкретный, указывается в IP. Пример ниже (где connect)
    local_addr.scope_id = 0;

    local_addr.addr.ipv4 = 0;
    // TODO(end)
    local_addr.port = 0;

    // Привязываем
    status = net_socket_bind(sock, &local_addr);
    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] bind failed: %d\n", status);
        net_socket_close(sock);
        return -1;
    }
    printk(KERN_INFO "[driver] Bind OK\n");

    net_address_t remote_addr; // удалённый адрес
    // TODO(begin): net_address_parse("{ip сервера (как v4, так и v6)}", debug_family, &local_addr);
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.family = debug_family;

    // scope_id для connect парсер должен получать САМ из IP!!!
    // делать через функцию dev_get_ifindex (#include <linux/netdevice.h>)
    // интерфейс пишется после IP через %.
    // Например, fe80::2889:bf2e:df6c:1e81%ens33 - этот ens33 сохраняем в буфер
    // и кидаем его в функцию, функция его ищет и возвращает число - сохраняем его.
    // Если % нету - scope_id равен 0
    remote_addr.scope_id = 2;

    if (debug_family == NET_AF_INET4)
        in4_pton("192.168.203.1", -1, (u8*)&remote_addr.addr.ipv4, -1, NULL);
    else
        in6_pton("fe80::2889:bf2e:df6c:1e81", -1, (u8*)&remote_addr.addr.ipv6, -1, NULL);
    // TODO(end)
    remote_addr.port = htons(debug_port); // TODO: linux_net_htons()

    // Отправляем
    char* msg = "Hello, World!";
    size_t sent = 0;

    status = net_socket_connect(sock, &remote_addr);
    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] Connect failed: %d\n", status);
        net_socket_close(sock);
        return -1;
    }

    status = net_socket_send(sock, msg, strlen(msg), &sent);
    if (status != NET_SUCCESS)
        printk(KERN_ERR "[driver] send failed: %d\n", status);
    else
        printk(KERN_INFO "[driver] sent %zu bytes\n", sent);

    g_test_socket = sock;
    return 0;
}

static void __exit minimal_driver_exit(void)
{
    if (g_test_socket)
    {
        if (net_socket_close(g_test_socket) != NET_SUCCESS)
        {
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
MODULE_DESCRIPTION("Client Test Driver");
MODULE_VERSION("1.0");
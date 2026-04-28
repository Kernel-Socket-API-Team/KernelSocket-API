#include "../../../../include/ksockapi.h"
// второстпенное
#include <linux/inet.h>   // in4_pton
#include <linux/string.h> // memset

// сюда принимаем выходной сокет
static net_socket_t* g_test_socket = NULL;

// TCP: 9001 - IPv4
//      9002 - IPv6
// UDP: 9003 - IPv4
//      9004 - IPv6

static int __init minimal_driver_init(void)
{
    printk(KERN_INFO "[driver] Loading module...\n");

    net_socket_t* sock = NULL;

    // IP, protocol, sock_type
    net_error_t status = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &sock);

    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] Failed to create socket, error code: %d\n", status);
        return -1;
    }

    net_address_t local_addr; // локальный адрес

    // parse {
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.family = NET_AF_INET4;
    local_addr.scope_id = 0;
    local_addr.addr.ipv4 = 0;
    local_addr.port = htons(9001);
    // } parse

    // Привязываем
    status = net_socket_bind(sock, &local_addr);
    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] Bind failed: %d\n", status);
        net_socket_close(sock);
        return -1;
    }
    printk(KERN_INFO "[driver] Bind OK\n");

    net_socket_t* client = NULL;
    status = net_socket_accept(sock, &client);
    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] Accept failed: %d\n", status);
        net_socket_close(sock);
        return -1;
    }
    printk(KERN_INFO "[driver] Accept OK\n");

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    size_t received = 0;

    status = net_socket_receive(client, buffer, sizeof(buffer) - 1, NULL, &received);
    if (status != NET_SUCCESS)
        printk(KERN_ERR "[driver] Receive failed: %d\n", status);
    else
        printk(KERN_INFO "[driver] Received %zu bytes: %s\n", received, buffer);

    net_socket_close(client);

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
MODULE_DESCRIPTION("Single Threading Server Test Driver");
MODULE_VERSION("1.0");
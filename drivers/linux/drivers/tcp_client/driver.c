#include "../../../../include/ksockapi.h"
// второстпенное
#include <linux/string.h> // memset
#include <linux/inet.h>   // in4_pton

// сюда принимаем выходной сокет
static net_socket_t* g_test_socket = NULL;

static int __init minimal_driver_init(void) {
    printk(KERN_INFO "[driver] Loading module...\n");

    net_socket_t* sock = NULL;

    // IP, protocol, sock_type
    net_error_t status = net_socket_create(NET_AF_INET4,
        NET_PROTO_UDP,
        NET_SOCK_TYPE_UDP,
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
    status = net_socket_bind(sock, &addr);
    if (status != NET_SUCCESS) {
        printk(KERN_ERR "[driver] bind failed: %d\n", status);
        net_socket_close(sock);
        return -1;
    }
    printk(KERN_INFO "[driver] Bind OK\n");

    // Здесь необходим net_error_t net_address_parse(const char* str, 
    //                                              net_family_t ip_family, 
    //                                              net_address_t* addr)
    // Вместо реализации ниже
    
    // Заполняем remote_addr — куда слать
    // htonl/htons нужны здесь потому что мы заполняем вручную (TODO: ???)
    net_address_t remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.family = NET_AF_INET4;
    remote_addr.port = htons(9003);
    remote_addr.addr.ipv4 = 0;
    in4_pton("192.168.203.1", -1, (u8 *)&remote_addr.addr.ipv4, -1, NULL);

    // Отправляем
    char* msg = "Hello, World!";
    size_t sent = 0;

    status = net_socket_connect(sock, &remote_addr);
    if (status != NET_SUCCESS) {
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

static void __exit minimal_driver_exit(void) {
    if (g_test_socket) {
        if (net_socket_close(g_test_socket) != NET_SUCCESS) {
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
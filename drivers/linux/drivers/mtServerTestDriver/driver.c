#include "../../../../include/ksockapi.h"
// второстпенное
#include <linux/inet.h>   // in4_pton
#include <linux/string.h> // memset
#include <linux/kthread.h> // Многопоточность

struct thread_config
{
    net_family_t family;
    net_protocol_t proto;
    net_socket_type_t sock_type;
    u16 port;
    struct task_struct* thread;
    net_socket_t* server_socket;
};

static struct thread_config server_threads[4];

static int server_thread_proc(void* data) 
{
    struct thread_config* config = (struct thread_config*)data;
    u16 port = config->port; 
    net_family_t family = config->family;
    net_protocol_t proto = config->proto;
    net_socket_type_t sock_type = config->sock_type;

    net_error_t status;

    char buffer[256];
    size_t received;

    printk(KERN_INFO "[driver] Loading module...\n");

    // IP, protocol, sock_type
    status = net_socket_create(family, proto, sock_type, &config->server_socket);

    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] Failed to create socket, error code: %d\n", status);
        return -1;
    }

    net_address_t local_addr; // локальный адрес

    // parse {
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.family = config->family;
    if (config->family == NET_AF_INET4)
    {
        local_addr.addr.ipv4 = 0;
    }
    else
    {
        memset(local_addr.addr.ipv6, 0, 16);
    }
    local_addr.port = htons(port);
    // } parse

    // Привязываем
    status = net_socket_bind(config->server_socket, &local_addr);
    if (status != NET_SUCCESS)
    {
        printk(KERN_ERR "[driver] Bind failed: %d\n", status);
        net_socket_close(config->server_socket);
        config->server_socket = NULL;
        return -1;
    }
    printk(KERN_INFO "[driver] Server started on port %d\n", port);

    // Цикл потока
    while (!kthread_should_stop())
    {
        memset(buffer, 0, sizeof(buffer));
        received = 0;

        if (proto == NET_PROTO_TCP)
        {
            net_socket_t* client_socket = NULL;
            status = net_socket_accept(config->server_socket, &client_socket);
            if (status != NET_SUCCESS)
            {
                printk(KERN_ERR "[driver] Accept failed: %d\n", status);
                net_socket_close(config->server_socket);
                return -1;
            }
            printk(KERN_INFO "[driver] Server accepted client\n");

            status = net_socket_receive(client_socket, buffer, sizeof(buffer) - 1, NULL, &received);
            if (status != NET_SUCCESS)
                printk(KERN_ERR "[driver] Receive failed: %d\n", status);
            else
            {
                printk(KERN_INFO "[driver] Received %zu bytes: %s\n", received, buffer);
                
            }

            net_socket_close(client_socket);
        }
        else
        {
            status = net_socket_receive(config->server_socket, buffer, sizeof(buffer) - 1, NULL, &received);
            if (status != NET_SUCCESS)
                printk(KERN_ERR "[driver] Receive failed: %d\n", status);
            else
            {
                printk(KERN_INFO "[driver] Received %zu bytes: %s\n", received, buffer);
            }
        }
        // timeout
        schedule_timeout_interruptible(msecs_to_jiffies(10));
    }

    return 0;
}

static int __init minimal_driver_init(void)
{
    int i;
    char name[32];
    printk(KERN_INFO "[driver] Loading module...\n");

    // TCP IPv4 (9001)
    server_threads[0] =
        (struct thread_config){NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, 9001, NULL, NULL};
    // TCP IPv6 (9002)
    server_threads[1] =
        (struct thread_config){NET_AF_INET6, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, 9002, NULL, NULL};
    // UDP IPv4 (9003)
    server_threads[2] = (struct thread_config){NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, 9003, NULL, NULL};
    // UDP IPv6 (9004)
    server_threads[3] = (struct thread_config){NET_AF_INET6, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, 9004, NULL, NULL};

    for (i = 0; i < 4; i++)
    {
        snprintf(name, sizeof(name), "kserv_%s_v%d", (server_threads[i].proto == NET_PROTO_TCP ? "tcp" : "udp"),
                 (server_threads[i].family == NET_AF_INET4 ? 4 : 6));

        server_threads[i].thread = kthread_run(server_thread_proc, &server_threads[i], name);
        if (IS_ERR(server_threads[i].thread))
        {
            printk(KERN_ERR "[driver] Failed to spawn thread %s\n", name);
            server_threads[i].thread = NULL;
        }
    }

    return 0;
}

static void __exit minimal_driver_exit(void)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        // Останавливаем поток
        if (server_threads[i].thread)
        {
            kthread_stop(server_threads[i].thread);
        }
        // Закрываем основной слушающий сокет
        if (server_threads[i].server_socket)
        {
            net_socket_close(server_threads[i].server_socket);
            printk(KERN_INFO "[driver] Socket on port %d closed\n", server_threads[i].port);
        }
    }
    printk(KERN_INFO "[driver] Module unloaded\n");
}

// Регистрируем функции загрузки и выгрузки
module_init(minimal_driver_init);
module_exit(minimal_driver_exit);

// Обязательная информация о модуле
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vyacheslav");
MODULE_DESCRIPTION("Multi Threading Server Test Driver");
MODULE_VERSION("1.0");
#include "../../../include/ksockapi.h"
#include <linux/kthread.h>
#include <linux/delay.h>

#define PORT_TCP 4444
#define PORT_UDP 4445
#define BUFFER_SIZE 1024

static bool g_Running = true;
static struct task_struct* g_WorkerThreadTCP = NULL;
static struct task_struct* g_WorkerThreadUDP = NULL;
static net_socket_t* g_ServerSockTCP = NULL;
static net_socket_t* g_ServerSockUDP = NULL;

// TCP серверный поток
static int ServerThreadTCP(void* Context)
{
    net_error_t err;
    net_address_t addr;
    net_socket_t* client_sock = NULL;
    char buffer[BUFFER_SIZE];
    size_t received;

    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
        goto exit;

    err = net_socket_create(NET_AF_INET6, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &g_ServerSockTCP);
    if (err != NET_SUCCESS)
        goto exit;

    err = net_address_parse("::", NET_AF_INET6, &addr);
    net_htons(PORT_TCP, &addr.port);
    if (err != NET_SUCCESS)
        goto close_server;

    err = net_socket_bind(g_ServerSockTCP, &addr);
    if (err != NET_SUCCESS)
        goto close_server;

    printk(KERN_INFO "[TCP] Server ready on port %d\n", PORT_TCP);

    while (g_Running && !kthread_should_stop())
    {
        err = net_socket_accept(g_ServerSockTCP, &client_sock);
        if (err != NET_SUCCESS || !g_Running)
            continue;

        printk(KERN_INFO "[TCP] Client connected\n");

        while (g_Running && !kthread_should_stop())
        {
            received = 0;
            net_address_t client_addr;
            err = net_socket_receive(client_sock, buffer, BUFFER_SIZE - 1, &client_addr, &received);

            if (err != NET_SUCCESS || received == 0)
                break;

            buffer[received] = '\0';

            // Эхо-ответ
            size_t sent;
            net_socket_send(client_sock, buffer, received, &sent);

            char ip_str[NET_ADDRSTRLEN];
            net_address_to_string(&client_addr, ip_str, sizeof(ip_str), 1);
            printk(KERN_INFO "[TCP] [%s] >> %s\n", ip_str, buffer);
        }

        net_socket_close(client_sock);
        client_sock = NULL;
        printk(KERN_INFO "[TCP] Client disconnected\n");
    }

close_server:
    if (g_ServerSockTCP)
    {
        net_socket_close(g_ServerSockTCP);
        g_ServerSockTCP = NULL;
    }

exit:
    printk(KERN_INFO "[TCP] Server thread exiting\n");
    return 0;
}

// UDP серверный поток
static int ServerThreadUDP(void* Context)
{
    net_error_t err;
    net_address_t addr;
    net_address_t client_addr;
    char buffer[BUFFER_SIZE];
    size_t received;

    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
        goto exit;

    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &g_ServerSockUDP);
    if (err != NET_SUCCESS)
        goto exit;

    err = net_address_parse("0.0.0.0", NET_AF_INET4, &addr);
    net_htons(PORT_UDP, &addr.port);
    if (err != NET_SUCCESS)
        goto close_socket;

    err = net_socket_bind(g_ServerSockUDP, &addr);
    if (err != NET_SUCCESS)
        goto close_socket;

    printk(KERN_INFO "[UDP] Server ready on port %d\n", PORT_UDP);

    while (g_Running && !kthread_should_stop())
    {
        received = 0;
        err = net_socket_receive(g_ServerSockUDP, buffer, BUFFER_SIZE - 1, &client_addr, &received);

        if (err != NET_SUCCESS || received == 0)
            continue;

        buffer[received] = '\0';

        // Эхо-ответ
        size_t sent;
        err = net_socket_connect(g_ServerSockUDP, &client_addr);
        if (err == NET_SUCCESS)
            net_socket_send(g_ServerSockUDP, buffer, received, &sent);

        char ip_str[NET_ADDRSTRLEN];
        net_address_to_string(&client_addr, ip_str, sizeof(ip_str), 1);
        printk(KERN_INFO "[UDP] [%s] >> %s\n", ip_str, buffer);
    }

close_socket:
    if (g_ServerSockUDP)
    {
        net_socket_close(g_ServerSockUDP);
        g_ServerSockUDP = NULL;
    }

exit:
    printk(KERN_INFO "[UDP] Server thread exiting\n");
    return 0;
}

// Выгрузка модуля
static void __exit echo_server_exit(void)
{
    g_Running = false;

    if (g_WorkerThreadTCP)
    {
        kthread_stop(g_WorkerThreadTCP);
        g_WorkerThreadTCP = NULL;
    }

    if (g_WorkerThreadUDP)
    {
        kthread_stop(g_WorkerThreadUDP);
        g_WorkerThreadUDP = NULL;
    }

    if (g_ServerSockTCP)
    {
        net_socket_close(g_ServerSockTCP);
        g_ServerSockTCP = NULL;
    }

    if (g_ServerSockUDP)
    {
        net_socket_close(g_ServerSockUDP);
        g_ServerSockUDP = NULL;
    }

    net_cleanup();
    printk(KERN_INFO "[ECHO] Module unloaded\n");
}

// Загрузка модуля
static int __init echo_server_init(void)
{
    if (net_register() != NET_SUCCESS)
        return -EIO;

    // TCP поток
    g_WorkerThreadTCP = kthread_run(ServerThreadTCP, NULL, "echo_tcp");
    if (IS_ERR(g_WorkerThreadTCP))
    {
        net_cleanup();
        return PTR_ERR(g_WorkerThreadTCP);
    }

    // UDP поток
    g_WorkerThreadUDP = kthread_run(ServerThreadUDP, NULL, "echo_udp");
    if (IS_ERR(g_WorkerThreadUDP))
    {
        kthread_stop(g_WorkerThreadTCP);
        g_WorkerThreadTCP = NULL;
        net_cleanup();
        return PTR_ERR(g_WorkerThreadUDP);
    }

    printk(KERN_INFO "[ECHO] Module loaded (TCP:%d, UDP:%d)\n", PORT_TCP, PORT_UDP);
    return 0;
}

module_init(echo_server_init);
module_exit(echo_server_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("TCP/UDP Echo Server using Kernel Socket API");
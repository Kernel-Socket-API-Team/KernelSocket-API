#include "../../../include/ksockapi.h"
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#define SERVER_PORT_TCP 9001
#define SERVER_PORT_UDP 9003
#define BUFFER_SIZE 1024
#define WAIT_TIMEOUT_MS 5000

// Параметры модуля
static char* server_ip = "192.168.68.1";
module_param(server_ip, charp, 0644);
MODULE_PARM_DESC(server_ip, "Server IP address");

static bool g_Running = true;
static struct task_struct* g_WorkerThreadClient = NULL;

// Функция отправки UDP сообщения
static net_error_t SendUdpMessage(const char* ip, uint16_t port, const char* message)
{
    net_error_t err;
    net_socket_t* sock = NULL;
    net_address_t server_addr;
    size_t sent;

    printk(KERN_INFO "[UDP Client] Sending to %s:%d: %s\n", ip, port, message);

    // Создание UDP сокета
    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &sock);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[UDP Client] Socket creation failed: %d\n", err);
        return err;
    }

    // Парсинг адреса сервера
    err = net_address_parse(ip, NET_AF_INET4, &server_addr);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[UDP Client] Address parse failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }
    net_htons(port, &server_addr.port);

    // Подключение к серверу (для UDP это просто сохранение адреса)
    err = net_socket_connect(sock, &server_addr);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[UDP Client] Connect failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    // Отправка сообщения
    err = net_socket_send(sock, message, strlen(message), &sent);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[UDP Client] Send failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    printk(KERN_INFO "[UDP Client] Sent %zu bytes\n", sent);

    // Получение ответа
    char buffer[BUFFER_SIZE];
    size_t received;
    err = net_socket_receive(sock, buffer, BUFFER_SIZE - 1, NULL, &received);
    if (err == NET_SUCCESS && received > 0)
    {
        buffer[received] = '\0';
        printk(KERN_INFO "[UDP Client] Received echo: %s\n", buffer);
    }

    net_socket_close(sock);
    return NET_SUCCESS;
}

// Функция отправки TCP сообщения
static net_error_t SendTcpMessage(const char* ip, uint16_t port, const char* message)
{
    net_error_t err;
    net_socket_t* sock = NULL;
    net_address_t server_addr;
    size_t sent, received;
    char buffer[BUFFER_SIZE];

    printk(KERN_INFO "[TCP Client] Connecting to %s:%d\n", ip, port);

    // Создание TCP сокета
    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[TCP Client] Socket creation failed: %d\n", err);
        return err;
    }

    // Парсинг адреса сервера
    err = net_address_parse(ip, NET_AF_INET4, &server_addr);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[TCP Client] Address parse failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }
    net_htons(port, &server_addr.port);

    // Подключение к серверу
    err = net_socket_connect(sock, &server_addr);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[TCP Client] Connect failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    printk(KERN_INFO "[TCP Client] Connected to %s:%d\n", ip, port);

    // Отправка сообщения
    err = net_socket_send(sock, message, strlen(message), &sent);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[TCP Client] Send failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    printk(KERN_INFO "[TCP Client] Sent %zu bytes: %s\n", sent, message);

    // Получение ответа (эхо)
    err = net_socket_receive(sock, buffer, BUFFER_SIZE - 1, NULL, &received);
    if (err == NET_SUCCESS && received > 0)
    {
        buffer[received] = '\0';
        printk(KERN_INFO "[TCP Client] Received echo: %s\n", buffer);
    }

    net_socket_close(sock);
    return NET_SUCCESS;
}

// Клиентский поток - последовательная отправка UDP и TCP
static int ClientThread(void* Context)
{
    net_error_t err;

    // Активация библиотеки
    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
    {
        printk(KERN_ERR "[Client] Activation failed: %d\n", err);
        goto exit;
    }

    printk(KERN_INFO "[Client] Starting test sequence to server: %s\n", server_ip);

    // Отправка UDP сообщения
    err = SendUdpMessage(server_ip, SERVER_PORT_UDP, "Hello from UDP client!");
    if (err != NET_SUCCESS)
        printk(KERN_ERR "[Client] UDP send failed: %d\n", err);

    // Небольшая задержка между отправками
    msleep(500);

    // Отправка TCP сообщения
    err = SendTcpMessage(server_ip, SERVER_PORT_TCP, "Hello from TCP client!");
    if (err != NET_SUCCESS)
        printk(KERN_ERR "[Client] TCP send failed: %d\n", err);

exit:
    printk(KERN_INFO "[Client] Thread exiting\n");
    return 0;
}

// Выгрузка модуля
static void __exit echo_client_exit(void)
{
    g_Running = false;

    // Поток сам завершился, не нужно его останавливать
    // Просто обнуляем указатель
    if (g_WorkerThreadClient)
    {
        g_WorkerThreadClient = NULL;
    }

    net_cleanup();
    printk(KERN_INFO "[Client] Module unloaded\n");
}
// Загрузка модуля
static int __init echo_client_init(void)
{
    // Регистрация библиотеки
    if (net_register() != NET_SUCCESS)
        return -EIO;

    // Создание клиентского потока
    g_WorkerThreadClient = kthread_run(ClientThread, NULL, "echo_client");
    if (IS_ERR(g_WorkerThreadClient))
    {
        net_cleanup();
        return PTR_ERR(g_WorkerThreadClient);
    }

    printk(KERN_INFO "[Client] Module loaded - will send UDP to port %d and TCP to port %d\n", 
           SERVER_PORT_UDP, SERVER_PORT_TCP);
    printk(KERN_INFO "[Client] Target server: %s\n", server_ip);

    return 0;
}

module_init(echo_client_init);
module_exit(echo_client_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("TCP/UDP Echo Client using Kernel Socket API");
MODULE_PARM_DESC(server_ip, "Server IP address (default: 127.0.0.1)");
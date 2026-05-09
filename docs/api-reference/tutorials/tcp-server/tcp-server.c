#include "../../../../include/ksockapi.h"

#ifdef _WIN32
#include <ntstrsafe.h>
#define net_snprintf RtlStringCbPrintfA
#else
#define net_snprintf snprintf
#endif

typedef struct
{
    volatile int should_stop;
    net_socket_t* listen_sock; // Слушающий сокет
    net_socket_t* client_sock; // Клиентский сокет
} thread_context_t;

static thread_context_t g_ctx = {0, NULL, NULL};

static int tcp_server_thread(void* data)
{
    thread_context_t* ctx = (thread_context_t*)data;
    net_error_t err;
    net_address_t local_addr;
    net_address_t client_addr;
    char buffer[256];
    char response[256];
    size_t received_len;
    size_t sent_len;
    char client_ip[NET_ADDRSTRLEN];
    
    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
        return -1;

    // Создание слушающего сокета (IPv6)
    err = net_socket_create(NET_AF_INET6, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &ctx->listen_sock);
    if (err != NET_SUCCESS)
    {
        net_cleanup();
        return -1;
    }

    // Привязка к IPv6 адресу (порт 8888)
    err = net_address_parse("[::0]:8888", NET_AF_INET6, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->listen_sock);
        ctx->listen_sock = NULL;
        net_cleanup();
        return -1;
    }

    err = net_socket_bind(ctx->listen_sock, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->listen_sock);
        ctx->listen_sock = NULL;
        net_cleanup();
        return -1;
    }

    // Основной цикл приёма клиентов
    while (!ctx->should_stop)
    {
        // Ожидание подключения (блокирующий вызов для каждого нового клиента)
        err = net_socket_accept(ctx->listen_sock, &ctx->client_sock);
        if (err != NET_SUCCESS) continue;  // Ошибка accept, пробуем снова

        // Получаем адрес клиента
        err = net_socket_get_remote_address(ctx->client_sock, &client_addr);
        if (err != NET_SUCCESS)
        {

            net_socket_close(ctx->client_sock);
            ctx->client_sock = NULL;
            continue;
        }
        
        net_address_to_string(&client_addr, client_ip, sizeof(client_ip), 1);
        
        while (!ctx->should_stop)
        {
            
            err = net_socket_receive(ctx->client_sock, buffer, sizeof(buffer) - 1, NULL, &received_len);
            
            // Выход при ошибке ИЛИ при штатном закрытии соединения (0 байт)
            if (err != NET_SUCCESS || received_len == 0) break;

            if (received_len > 0)
            {
                buffer[received_len] = '\0';
                
                // Формируем ответ с указанием IP отправителя
                net_snprintf(response, sizeof(response), "[Client %s]: %s\n", client_ip, buffer);

                err = net_socket_send(ctx->client_sock, response, strlen(response), &sent_len);
            }
        }

        // Закрываем соединение с текущим клиентом
        if (ctx->client_sock)
        {
            net_socket_close(ctx->client_sock);
            ctx->client_sock = NULL;
        }
    
    }

    // Закрытие слушающего сокета
    if (ctx->listen_sock)
    {
        net_socket_close(ctx->listen_sock);
        ctx->listen_sock = NULL;
    }

    return 0;
}

// Точка входа зависит от платформы
#ifdef _WIN32

// Windows: DriverEntry
static PETHREAD g_WorkerThread = NULL;

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    g_ctx.should_stop = 1;

    if (g_ctx.client_sock)
    {
        net_socket_close(g_ctx.client_sock);
        g_ctx.client_sock = NULL;
    }

    if (g_ctx.listen_sock)
    {
        net_socket_close(g_ctx.listen_sock);
        g_ctx.listen_sock = NULL;
    }

    if (g_WorkerThread)
    {
        KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_WorkerThread);
        g_WorkerThread = NULL;
    }

    net_cleanup();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    HANDLE hThread;
    NTSTATUS status;

    DriverObject->DriverUnload = DriverUnload;

    status = net_register();
    if (status != NET_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status =
        PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, (PKSTART_ROUTINE)tcp_server_thread, &g_ctx);
    if (!NT_SUCCESS(status))
    {
        net_cleanup();
        return status;
    }

    status =
        ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID*)&g_WorkerThread, NULL);
    ZwClose(hThread);

    if (!NT_SUCCESS(status))
    {
        g_ctx.should_stop = 1;
        KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
        net_cleanup();
        return status;
    }

    return STATUS_SUCCESS;
}

#else

// Linux: module_init
static struct task_struct* g_WorkerThread = NULL;

static int __init tcp_server_module_init(void)
{
    net_error_t err;

    err = net_register();
    if (err != NET_SUCCESS)
        return -EIO;

    g_WorkerThread = kthread_run(tcp_server_thread, &g_ctx, "tcp_server");
    if (IS_ERR(g_WorkerThread))
    {
        net_cleanup();
        return PTR_ERR(g_WorkerThread);
    }

    return 0;
}

static void __exit tcp_server_module_exit(void)
{
    g_ctx.should_stop = 1;

    if (g_WorkerThread)
    {
        kthread_stop(g_WorkerThread);
        g_WorkerThread = NULL;
    }

    if (g_ctx.listen_sock)
    {
        net_socket_close(g_ctx.listen_sock);
        g_ctx.listen_sock = NULL;
    }

    if (g_ctx.client_sock)
    {
        net_socket_close(g_ctx.client_sock);
        g_ctx.client_sock = NULL;
    }

    net_cleanup();
}

module_init(tcp_server_module_init);
module_exit(tcp_server_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TCP Echo Server for Kernel Socket API (single client)");
MODULE_AUTHOR("Your Name");
#endif
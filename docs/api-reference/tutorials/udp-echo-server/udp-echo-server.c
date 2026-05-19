#include "../../../../include/ksockapi.h"

typedef struct
{
    volatile int should_stop;
    net_socket_t* sock;
} thread_context_t;

static thread_context_t g_ctx = {0, NULL};

static int udp_echo_server_thread(void* data)
{
    thread_context_t* ctx = (thread_context_t*)data;
    net_error_t err;
    net_address_t local_addr;
    net_address_t client_addr;
    char buffer[256];
    size_t received_len;
    size_t sent_len;
    char addr_str[NET_ADDRSTRLEN];

    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
        return -1;

    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &ctx->sock);
    if (err != NET_SUCCESS)
    {
        net_cleanup();
        return -1;
    }

    err = net_address_parse("0.0.0.0:8888", NET_AF_INET4, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->sock);
        ctx->sock = NULL;
        net_cleanup();
        return -1; 
    }

    err = net_socket_bind(ctx->sock, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->sock);
        ctx->sock = NULL;
        net_cleanup();
        return -1; 
    }

    while (!ctx->should_stop)
    {
        err = net_socket_receive(ctx->sock, buffer, sizeof(buffer), &client_addr, &received_len);

        if (err != NET_SUCCESS)
            continue;

        // Преобразуем адрес клиента в строку
        net_address_to_string(&client_addr, addr_str, sizeof(addr_str), 0);

        // Выводим содержимое пакета (первые 20 байт)
        if (received_len > 0)
            buffer[received_len] = '\0';

        err = net_socket_connect(ctx->sock, &client_addr);
        if (err != NET_SUCCESS)
            continue;

        err = net_socket_send(ctx->sock, buffer, received_len, &sent_len);
    }

    net_cleanup();

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

    // Принудительно закрываем сокет, чтобы прервать receive
    if (g_ctx.sock)
    {
        net_socket_close(g_ctx.sock);
        g_ctx.sock = NULL;
    }

    if (g_WorkerThread)
    {
        KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_WorkerThread);
        g_WorkerThread = NULL;
    }
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

    status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                  (PKSTART_ROUTINE)udp_echo_server_thread, &g_ctx);
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

static int __init udp_echo_module_init(void)
{
    net_error_t err;

    err = net_register();
    if (err != NET_SUCCESS)
        return -EIO;

    g_WorkerThread = kthread_run(udp_echo_server_thread, &g_ctx, "udp_echo_server");
    if (IS_ERR(g_WorkerThread))
    {
        net_cleanup();
        return PTR_ERR(g_WorkerThread);
    }

    return 0;
}

static void __exit udp_echo_module_exit(void)
{
    g_ctx.should_stop = 1;

    if (g_WorkerThread)
    {
        kthread_stop(g_WorkerThread);
        g_WorkerThread = NULL;
    }

    if (g_ctx.sock)
    {
        net_socket_close(g_ctx.sock);
        g_ctx.sock = NULL;
    }
}

module_init(udp_echo_module_init);
module_exit(udp_echo_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("UDP Echo Server for Kernel Socket API");
MODULE_AUTHOR("Your Name");
#endif
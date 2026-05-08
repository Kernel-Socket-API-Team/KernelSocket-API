#include <ksockapi.h>

#define PORT_TCP 4444
#define PORT_UDP 4445
#define BUFFER_SIZE 1024

volatile BOOLEAN g_Running = TRUE;
PETHREAD g_WorkerThreadTCP = NULL;
PETHREAD g_WorkerThreadUDP = NULL;
net_socket_t* g_ServerSockTCP = NULL;
net_socket_t* g_ServerSockUDP = NULL;

// TCP серверный поток
VOID ServerThreadTCP(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    net_error_t err;
    net_address_t addr;
    net_socket_t* client_sock = NULL;
    char buffer[BUFFER_SIZE];
    size_t received;

    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
        goto exit;

    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &g_ServerSockTCP);
    if (err != NET_SUCCESS)
        goto exit;

    err = net_address_parse("0.0.0.0", NET_AF_INET4, &addr);
    net_htons(PORT_TCP, &addr.port);
    if (err != NET_SUCCESS)
        goto close_server;

    err = net_socket_bind(g_ServerSockTCP, &addr);
    if (err != NET_SUCCESS)
        goto close_server;

    DbgPrint("[TCP] Server ready on port %d\n", PORT_TCP);

    while (g_Running)
    {
        err = net_socket_accept(g_ServerSockTCP, &client_sock);
        if (err != NET_SUCCESS || !g_Running)
            continue;

        DbgPrint("[TCP] Client connected\n");

        while (g_Running)
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
            net_address_to_string(&client_addr, ip_str, sizeof(ip_str), TRUE);
            DbgPrint("[TCP] [%s] >> %s\n", ip_str, buffer);
        }

        net_socket_close(client_sock);
        client_sock = NULL;
        DbgPrint("[TCP] Client disconnected\n");
    }

close_server:
    if (g_ServerSockTCP)
    {
        net_socket_close(g_ServerSockTCP);
        g_ServerSockTCP = NULL;
    }

exit:
    DbgPrint("[TCP] Server thread exiting\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

// UDP серверный поток
VOID ServerThreadUDP(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
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

    DbgPrint("[UDP] Server ready on port %d\n", PORT_UDP);

    while (g_Running)
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
        net_address_to_string(&client_addr, ip_str, sizeof(ip_str), TRUE);
        DbgPrint("[UDP] [%s] >> %s\n", ip_str, buffer);
    }

close_socket:
    if (g_ServerSockUDP)
    {
        net_socket_close(g_ServerSockUDP);
        g_ServerSockUDP = NULL;
    }

exit:
    DbgPrint("[UDP] Server thread exiting\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

// Выгрузка драйвера
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    g_Running = FALSE;

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

    if (g_WorkerThreadTCP)
    {
        KeWaitForSingleObject(g_WorkerThreadTCP, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_WorkerThreadTCP);
        g_WorkerThreadTCP = NULL;
    }

    if (g_WorkerThreadUDP)
    {
        KeWaitForSingleObject(g_WorkerThreadUDP, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_WorkerThreadUDP);
        g_WorkerThreadUDP = NULL;
    }

    net_cleanup();
    DbgPrint("[ECHO] Driver unloaded\n");
}

// Точка входа
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = DriverUnload;

    if (net_register() != NET_SUCCESS)
        return STATUS_UNSUCCESSFUL;

    NTSTATUS status;
    HANDLE hThread;

    // TCP поток
    status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, ServerThreadTCP, NULL);
    if (!NT_SUCCESS(status))
    {
        net_cleanup();
        return status;
    }
    ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID*)&g_WorkerThreadTCP, NULL);
    ZwClose(hThread);

    // UDP поток
    status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, ServerThreadUDP, NULL);
    if (!NT_SUCCESS(status))
    {
        net_cleanup();
        return status;
    }
    ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID*)&g_WorkerThreadUDP, NULL);
    ZwClose(hThread);

    DbgPrint("[ECHO] Driver loaded (TCP:%d, UDP:%d)\n", PORT_TCP, PORT_UDP);
    return STATUS_SUCCESS;
}
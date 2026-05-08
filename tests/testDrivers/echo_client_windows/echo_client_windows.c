#include <ksockapi.h>

#define SERVER_PORT_TCP 9001
#define SERVER_PORT_UDP 9003
#define BUFFER_SIZE 1024
#define WAIT_TIMEOUT_MS 5000

// Глобальные переменные
volatile BOOLEAN g_Running = TRUE;
PETHREAD g_WorkerThreadClient = NULL;

// Функция отправки UDP сообщения
net_error_t SendUdpMessage(const char* server_ip, uint16_t port, const char* message)
{
    net_error_t err;
    net_socket_t* sock = NULL;
    net_address_t server_addr;
    size_t sent;

    DbgPrint("[UDP Client] Sending to %s:%d: %s\n", server_ip, port, message);

    // Создание UDP сокета
    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &sock);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[UDP Client] Socket creation failed: %d\n", err);
        return err;
    }

    // Парсинг адреса сервера
    err = net_address_parse(server_ip, NET_AF_INET4, &server_addr);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[UDP Client] Address parse failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }
    net_htons(port, &server_addr.port);

    // Подключение к серверу (для UDP это просто сохранение адреса)
    err = net_socket_connect(sock, &server_addr);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[UDP Client] Connect failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    // Отправка сообщения
    err = net_socket_send(sock, message, strlen(message), &sent);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[UDP Client] Send failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    DbgPrint("[UDP Client] Sent %zu bytes\n", sent);

    // Получение ответа (опционально)
    char buffer[BUFFER_SIZE];
    size_t received;
    err = net_socket_receive(sock, buffer, BUFFER_SIZE - 1, NULL, &received);
    if (err == NET_SUCCESS && received > 0)
    {
        buffer[received] = '\0';
        DbgPrint("[UDP Client] Received echo: %s\n", buffer);
    }

    net_socket_close(sock);
    return NET_SUCCESS;
}

// Функция отправки TCP сообщения
net_error_t SendTcpMessage(const char* server_ip, uint16_t port, const char* message)
{
    net_error_t err;
    net_socket_t* sock = NULL;
    net_address_t server_addr;
    size_t sent, received;
    char buffer[BUFFER_SIZE];

    DbgPrint("[TCP Client] Connecting to %s:%d\n", server_ip, port);

    // Создание TCP сокета
    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TCP Client] Socket creation failed: %d\n", err);
        return err;
    }

    // Парсинг адреса сервера
    err = net_address_parse(server_ip, NET_AF_INET4, &server_addr);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TCP Client] Address parse failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }
    net_htons(port, &server_addr.port);

    // Подключение к серверу
    err = net_socket_connect(sock, &server_addr);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TCP Client] Connect failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    DbgPrint("[TCP Client] Connected to %s:%d\n", server_ip, port);

    // Отправка сообщения
    err = net_socket_send(sock, message, strlen(message), &sent);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TCP Client] Send failed: %d\n", err);
        net_socket_close(sock);
        return err;
    }

    DbgPrint("[TCP Client] Sent %zu bytes: %s\n", sent, message);

    // Получение ответа (эхо)
    err = net_socket_receive(sock, buffer, BUFFER_SIZE - 1, NULL, &received);
    if (err == NET_SUCCESS && received > 0)
    {
        buffer[received] = '\0';
        DbgPrint("[TCP Client] Received echo: %s\n", buffer);
    }

    net_socket_close(sock);
    return NET_SUCCESS;
}

// Клиентский поток - последовательная отправка UDP и TCP
VOID ClientThread(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    net_error_t err;

    // Активация библиотеки
    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[Client] Activation failed: %d\n", err);
        goto exit;
    }

    DbgPrint("[Client] Starting test sequence...\n");

    // Отправка UDP сообщения
    err = SendUdpMessage("192.168.68.1", SERVER_PORT_UDP, "Hello from UDP client!");
    if (err != NET_SUCCESS)
        DbgPrint("[Client] UDP send failed: %d\n", err);

    // Небольшая задержка между отправками
    LARGE_INTEGER delay;
    delay.QuadPart = -500 * 10000; // 500ms
    KeDelayExecutionThread(KernelMode, FALSE, &delay);

    // Отправка TCP сообщения
    err = SendTcpMessage("192.168.68.1", SERVER_PORT_TCP, "Hello from TCP client!");
    if (err != NET_SUCCESS)
        DbgPrint("[Client] TCP send failed: %d\n", err);

    DbgPrint("[Client] Test sequence completed\n");

exit:
    DbgPrint("[Client] Thread exiting\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

// Выгрузка драйвера
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    g_Running = FALSE;

    if (g_WorkerThreadClient)
    {
        KeWaitForSingleObject(g_WorkerThreadClient, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_WorkerThreadClient);
        g_WorkerThreadClient = NULL;
    }

    net_cleanup();
    DbgPrint("[Client] Driver unloaded\n");
}

// Точка входа
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = DriverUnload;

    // Регистрация библиотеки
    if (net_register() != NET_SUCCESS)
        return STATUS_UNSUCCESSFUL;

    NTSTATUS status;
    HANDLE hThread;

    // Создание клиентского потока
    status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, ClientThread, NULL);
    if (!NT_SUCCESS(status))
    {
        net_cleanup();
        return status;
    }

    ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID*)&g_WorkerThreadClient,
                              NULL);
    ZwClose(hThread);

    DbgPrint("[Client] Driver loaded - will send UDP to port %d and TCP to port %d\n", SERVER_PORT_UDP,
             SERVER_PORT_TCP);

    return STATUS_SUCCESS;
}
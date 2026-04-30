#include <ksockapi.h>

// HANDLE рабочего потока
static PETHREAD g_WorkerThread = NULL;

#define TEST_LOG(fmt, ...) DbgPrint("[KSOCK-TEST] " fmt "\n", __VA_ARGS__)

// =====================================================================
// TCP-тест: connect к 127.0.0.1:9000 + send "Hello, World!"
// На стороне виртуалки: ncat -l -p 9000
// =====================================================================
static VOID RunTcpTest(VOID)
{
    net_error_t err;
    net_socket_t* sock = NULL;
    net_address_t remote;
    size_t sent = 0;
    const char* message = "Hello, World! (TCP)";
    size_t message_len = 19;

    TEST_LOG("--- TCP test start ---");

    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("TCP FAIL: net_socket_create = %d", (int)err);
        return;
    }
    TEST_LOG("TCP OK: socket created (sock=%p)", sock);

    RtlZeroMemory(&remote, sizeof(remote));
    remote.family = NET_AF_INET4;
    remote.addr.ipv4 = 0x0100007F; // 127.0.0.1 в сетевом порядке байт

    err = net_htons(9000, &remote.port);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("TCP FAIL: net_htons = %d", (int)err);
        net_socket_close(sock);
        return;
    }

    TEST_LOG("TCP: connecting to 127.0.0.1:9000...");
    err = net_socket_connect(sock, &remote);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("TCP FAIL: net_socket_connect = %d", (int)err);
        net_socket_close(sock);
        return;
    }
    TEST_LOG("TCP OK: connection established");

    err = net_socket_send(sock, message, message_len, &sent);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("TCP FAIL: net_socket_send = %d", (int)err);
        net_socket_close(sock);
        return;
    }
    TEST_LOG("TCP OK: sent %llu bytes ('%s')", (ULONG64)sent, message);

    err = net_socket_close(sock);
    if (err != NET_SUCCESS)
        TEST_LOG("TCP WARN: net_socket_close = %d", (int)err);
    else
        TEST_LOG("TCP OK: socket closed");

    TEST_LOG("--- TCP test end ---");
}

// =====================================================================
// UDP-тест: send датаграммы на 192.168.218.1:9003
// (тот же адрес и порт, что в твоём референс-драйвере)
// На стороне получателя: ncat -u -l -p 9003
// =====================================================================
static VOID RunUdpTest(VOID)
{
    net_error_t err;
    net_socket_t* sock = NULL;
    net_address_t local;
    net_address_t remote;
    size_t sent = 0;
    const char* message = "Hello, World! (UDP)";
    size_t message_len = 19;

    TEST_LOG("--- UDP test start ---");

    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &sock);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("UDP FAIL: net_socket_create = %d", (int)err);
        return;
    }
    TEST_LOG("UDP OK: socket created (sock=%p)", sock);

    // Bind на ANY:0 — как в референс-драйвере (явный bind, чтобы не полагаться на implicit)
    RtlZeroMemory(&local, sizeof(local));
    local.family = NET_AF_INET4;
    local.addr.ipv4 = 0; // INADDR_ANY
    local.port = 0;

    err = net_socket_bind(sock, &local);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("UDP FAIL: net_socket_bind = %d", (int)err);
        net_socket_close(sock);
        return;
    }
    TEST_LOG("UDP OK: bound to ANY:0");

    // Адрес назначения: 192.168.218.1:9003
    RtlZeroMemory(&remote, sizeof(remote));
    remote.family = NET_AF_INET4;

    // 192.168.218.1 в сетевом порядке (little-endian машина):
    // байты: 192=0xC0, 168=0xA8, 218=0xDA, 1=0x01
    // в network byte order первым идёт 192, поэтому в uint32 little-endian = 0x01DAA8C0
    remote.addr.ipv4 = 0x01DAA8C0;

    err = net_htons(9003, &remote.port);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("UDP FAIL: net_htons = %d", (int)err);
        net_socket_close(sock);
        return;
    }

    // Для UDP connect просто сохраняет адрес внутри сокета
    err = net_socket_connect(sock, &remote);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("UDP FAIL: net_socket_connect = %d", (int)err);
        net_socket_close(sock);
        return;
    }
    TEST_LOG("UDP OK: destination address saved (192.168.218.1:9003)");

    err = net_socket_send(sock, message, message_len, &sent);
    if (err != NET_SUCCESS)
    {
        TEST_LOG("UDP FAIL: net_socket_send = %d", (int)err);
        net_socket_close(sock);
        return;
    }
    TEST_LOG("UDP OK: sent %llu bytes ('%s')", (ULONG64)sent, message);

    err = net_socket_close(sock);
    if (err != NET_SUCCESS)
        TEST_LOG("UDP WARN: net_socket_close = %d", (int)err);
    else
        TEST_LOG("UDP OK: socket closed");

    TEST_LOG("--- UDP test end ---");
}

VOID NetworkWorkerThread(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    TEST_LOG("Worker thread started");

    net_error_t err = net_is_ready();

    if (err != NET_SUCCESS)
    {
        TEST_LOG("Library not ready, calling net_activate...");
        err = net_activate(NET_WAIT_INFINITE);
    }

    if (err == NET_SUCCESS)
    {
        TEST_LOG("Library ready, starting tests");

        // Небольшая пауза, чтобы успеть запустить ncat-серверы (если ещё не запущены)
        LARGE_INTEGER delay;
        delay.QuadPart = -30000000LL; // 3 секунды
        KeDelayExecutionThread(KernelMode, FALSE, &delay);

        RunTcpTest();

        // Пауза между тестами для удобства чтения логов
        delay.QuadPart = -10000000LL; // 1 секунда
        KeDelayExecutionThread(KernelMode, FALSE, &delay);

        RunUdpTest();

        TEST_LOG("All tests finished");
    }
    else if (err == NET_ERROR_TIMEOUT)
    {
        TEST_LOG("FAIL: net_activate timeout");
    }
    else
    {
        TEST_LOG("FAIL: net_activate/is_ready returned %d", (int)err);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    if (g_WorkerThread)
    {
        KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_WorkerThread);
        g_WorkerThread = NULL;
    }

    net_cleanup();
    TEST_LOG("Driver unloaded");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = DriverUnload;

    TEST_LOG("Driver loading...");

    net_error_t err = net_register();
    if (err != NET_SUCCESS)
    {
        TEST_LOG("FAIL: net_register = %d", (int)err);
        return STATUS_UNSUCCESSFUL;
    }
    TEST_LOG("net_register OK");

    HANDLE hThread;
    NTSTATUS Status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, NetworkWorkerThread, NULL);
    if (!NT_SUCCESS(Status))
    {
        net_cleanup();
        return Status;
    }

    Status =
        ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, PsThreadType, KernelMode, (PVOID)&g_WorkerThread, NULL);

    ZwClose(hThread);

    if (!NT_SUCCESS(Status))
    {
        net_cleanup();
        return Status;
    }

    return STATUS_SUCCESS;
}
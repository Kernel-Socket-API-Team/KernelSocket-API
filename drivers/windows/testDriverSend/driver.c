#include <ksockapi.h>

static PETHREAD g_TestThread = NULL;
static net_socket_t* g_Socket = NULL;

static VOID TestThread(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    net_error_t err;
    net_address_t addr;
    char msg[] = "Hello!";
    size_t sent;

    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TEST] net_activate failed: %d\n", err);
        goto exit;
    }

    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &g_Socket);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TEST] Create failed: %d\n", err);
        goto cleanup;
    }

    err = net_address_parse("192.168.68.1", NET_AF_INET4, &addr);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TEST] Parse failed: %d\n", err);
        goto close;
    }
    net_htons(9003, &addr.port);

    DbgPrint("[TEST] Connecting...\n");
    err = net_socket_connect(g_Socket, &addr);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TEST] Connect failed: %d\n", err);
        goto close;
    }
    DbgPrint("[TEST] Connected!\n");

    err = net_socket_send(g_Socket, msg, sizeof(msg), &sent);
    if (err != NET_SUCCESS)
    {
        DbgPrint("[TEST] Send failed: %d\n", err);
        goto close;
    }
    DbgPrint("[TEST] Sent %zu bytes\n", sent);

close:
    if (g_Socket)
        net_socket_close(g_Socket);
cleanup:
    net_cleanup();
exit:
    DbgPrint("[TEST] Thread exit\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    if (g_TestThread)
    {
        KeWaitForSingleObject(g_TestThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_TestThread);
    }
    net_cleanup();
    DbgPrint("[TEST] Unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    HANDLE hThread;
    NTSTATUS status;

    DriverObject->DriverUnload = DriverUnload;

    if (net_register() != NET_SUCCESS)
        return STATUS_UNSUCCESSFUL;

    status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, TestThread, NULL);
    if (!NT_SUCCESS(status))
    {
        net_cleanup();
        return status;
    }

    ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID*)&g_TestThread, NULL);
    ZwClose(hThread);

    DbgPrint("[TEST] Loaded\n");
    return STATUS_SUCCESS;
}
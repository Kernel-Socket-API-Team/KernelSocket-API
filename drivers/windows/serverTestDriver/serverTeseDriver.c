#include <ksockapi.h>
#include <ntddk.h>

#define PORT 4444
#define BUFFER_SIZE 1024

volatile BOOLEAN g_Running = TRUE;
PETHREAD g_WorkerThread = NULL;
net_socket_t *g_ServerSock = NULL;

VOID ServerThread(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

  net_error_t err;
  net_address_t addr;
  net_socket_t *client_sock = NULL;
  char buffer[BUFFER_SIZE];
  size_t received;

  // Инициализация
  if (net_is_ready() != NET_SUCCESS)
    net_activate(NET_WAIT_INFINITE);

  // Создание сокета
  net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_FLAG_LISTENING, &g_ServerSock);

  // Привязка
  addr.family = NET_AF_INET4;
  net_htons(4444, &addr.port);
  addr.addr.ipv4 = 0;
  net_socket_bind(g_ServerSock, &addr);

  DbgPrint("[SERVER] Ready on port %d\n", PORT);

  // Главный цикл сервера
  while (g_Running) {
    if (net_socket_accept(g_ServerSock, &client_sock) != NET_SUCCESS) continue;
    
    DbgPrint("[SERVER] + Client connected\n");

    while (1) {
      received = 0;
      err = net_socket_receive(client_sock, buffer, BUFFER_SIZE - 1, NULL, &received);

      if (err != NET_SUCCESS || received == 0) break;

      buffer[received] = '\0';
      DbgPrint("[SERVER] >> %s\n", buffer);
    }

    net_socket_close(client_sock);
    DbgPrint("[SERVER] - Client disconnected\n");
  }

  if (g_ServerSock) {
    net_socket_close(g_ServerSock);
    g_ServerSock = NULL;
  }

  PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  g_Running = FALSE;

  if (g_ServerSock) {
    net_socket_close(g_ServerSock);
    g_ServerSock = NULL;
  }

  if (g_WorkerThread) {
    KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(g_WorkerThread);
    g_WorkerThread = NULL;
  }

  net_cleanup();
  DbgPrint("[SERVER] Unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  if (net_register() != NET_SUCCESS) return STATUS_UNSUCCESSFUL;

  HANDLE hThread;
  NTSTATUS status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, ServerThread, NULL);
  if (!NT_SUCCESS(status)) {
    net_cleanup();
    return status;
  }

  ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID *)&g_WorkerThread, NULL);
  ZwClose(hThread);

  DbgPrint("[SERVER] Loaded\n");
  return STATUS_SUCCESS;
}
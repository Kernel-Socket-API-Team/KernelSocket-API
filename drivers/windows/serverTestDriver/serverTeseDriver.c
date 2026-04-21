#include <ksockapi.h>

#define PORT_TCP 4444
#define PORT_UDP 4445
#define BUFFER_SIZE 1024

volatile BOOLEAN g_Running = TRUE;

PETHREAD g_WorkerThreadTCP = NULL;
PETHREAD g_WorkerThreadUDP = NULL;

net_socket_t *g_ServerSockTCP = NULL;
net_socket_t *g_ServerSockUDP = NULL;

VOID ServerThreadTCP(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

  net_error_t err;
  net_address_t addr;
  net_socket_t *client_sock = NULL;
  char buffer[BUFFER_SIZE];
  size_t received;

  // Инициализация
  err = net_activate(NET_WAIT_INFINITE);
  if (err != NET_SUCCESS) goto exit;

  // Создание сокета
  err = net_socket_create(NET_AF_INET6, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN,
                          &g_ServerSockTCP);
  if (err != NET_SUCCESS) goto exit;

  // Привязка
  /*
  addr.family = NET_AF_INET6;
  net_htons(PORT_TCP, &addr.port);
  addr.addr.ipv4 = 0;*/

  // Привязка для TCP
  addr.family = NET_AF_INET6;
  net_htons(PORT_TCP, &addr.port);
  memset(addr.addr.ipv6, 0, 16); // :: - все IPv6 интерфейсы

  err = net_socket_bind(g_ServerSockTCP, &addr);
  if (err != NET_SUCCESS) goto close_server;

  DbgPrint("[SERVER_TCP] Ready on port %d\n", PORT_TCP);

  // Главный цикл сервера
  while (g_Running) {
    err = net_socket_accept(g_ServerSockTCP, &client_sock);
    if (err != NET_SUCCESS) {
      if (g_Running) DbgPrint("[SERVER_TCP] Accept error: %d\n", err);
      continue;
    }

    DbgPrint("[SERVER_TCP] + Client connected\n");

    // Цикл приема данных от клиента
    while (g_Running) {
      received = 0;
      net_address_t client_addr;
      err = net_socket_receive(client_sock, buffer, BUFFER_SIZE - 1, &client_addr, &received);

      if (err != NET_SUCCESS) {
        if (g_Running) DbgPrint("[SERVER_TCP] Receive error: %d\n", err);
        break;
      }

      if (received == 0) break; // Клиент отключился
      
      buffer[received] = '\0';
      char ip_str[NET_ADDRSTRLEN];
      net_address_to_string(&client_addr, ip_str, sizeof(ip_str), TRUE);

      DbgPrint("[SERVER_TCP] [%s] >> %s;\n", ip_str, buffer);
    }

    net_socket_close(client_sock);
    client_sock = NULL;
    DbgPrint("[SERVER_TCP] - Client disconnected\n");
  }

close_server:
  if (g_ServerSockTCP) {
    net_socket_close(g_ServerSockTCP);
    g_ServerSockTCP = NULL;
  }

exit:
  DbgPrint("[SERVER_TCP] Thread exiting\n");
  PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID ServerThreadUDP(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

  net_error_t err;
  net_address_t addr;
  net_address_t client_addr;
  char buffer[BUFFER_SIZE];
  size_t received = 0;

  // Инициализация
  err = net_activate(NET_WAIT_INFINITE);
  if (err != NET_SUCCESS) goto exit;

  // Создание UDP сокета
  err = net_socket_create(NET_AF_INET6, NET_PROTO_UDP, NET_SOCK_TYPE_UDP,
                          &g_ServerSockUDP);
  if (err != NET_SUCCESS) goto exit;

  // Привязка к порту
  /*
    addr.family = NET_AF_INET4;
    net_htons(PORT_UDP, &addr.port);
    addr.addr.ipv4 = 0;
  */
  addr.family = NET_AF_INET6;
  net_htons(PORT_UDP, &addr.port);
  memset(addr.addr.ipv6, 0, 16); // :: - все IPv6 интерфейсы

  err = net_socket_bind(g_ServerSockUDP, &addr);
  if (err != NET_SUCCESS) goto close_socket;

  DbgPrint("[SERVER_UDP] Ready on port %d\n", PORT_UDP);

  // Главный цикл сервера
  while (g_Running) {
    received = 0;
    err = net_socket_receive(g_ServerSockUDP, buffer, BUFFER_SIZE - 1, &client_addr, &received);

    if (err != NET_SUCCESS) {
      if (g_Running) DbgPrint("[SERVER_UDP] Receive error: %d\n", err);
      continue;
    }

    if (received > 0) {
      buffer[received] = '\0';
      char ip_str[NET_ADDRSTRLEN];
      net_address_to_string(&client_addr, ip_str, sizeof(ip_str), TRUE);

      DbgPrint("[SERVER_UDP] [%s] >> %s\n", ip_str, buffer);
    }
  }

close_socket:
  if (g_ServerSockUDP) {
    net_socket_close(g_ServerSockUDP);
    g_ServerSockUDP = NULL;
  }

exit:
  DbgPrint("[SERVER_UDP] Thread exiting\n");
  PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  g_Running = FALSE;

  if (g_ServerSockTCP) {
    net_socket_close(g_ServerSockTCP);
    g_ServerSockTCP = NULL;
  }
    
  if (g_ServerSockUDP) {
    net_socket_close(g_ServerSockUDP);
    g_ServerSockUDP = NULL;
  }

  if (g_WorkerThreadTCP) {
    KeWaitForSingleObject(g_WorkerThreadTCP, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(g_WorkerThreadTCP);
    g_WorkerThreadTCP = NULL;
  }

  if (g_WorkerThreadUDP) {
    KeWaitForSingleObject(g_WorkerThreadUDP, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(g_WorkerThreadUDP);
    g_WorkerThreadUDP = NULL;
  }

  net_cleanup();
  DbgPrint("[SERVER] Unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  if (net_register() != NET_SUCCESS) return STATUS_UNSUCCESSFUL;
  NTSTATUS status;

  HANDLE hThreadTCP;
  status = PsCreateSystemThread(&hThreadTCP, THREAD_ALL_ACCESS, NULL,
                                         NULL, NULL, ServerThreadTCP, NULL);
  if (!NT_SUCCESS(status)) {
    net_cleanup();
    return status;
  }

  ObReferenceObjectByHandle(hThreadTCP, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID *)&g_WorkerThreadTCP, NULL);
  ZwClose(hThreadTCP);
  DbgPrint("[SERVER] TCP Loaded!\n");
  
  HANDLE hThreadUDP;
  status = PsCreateSystemThread(&hThreadUDP, THREAD_ALL_ACCESS, NULL, NULL, NULL, ServerThreadUDP, NULL);

  if (!NT_SUCCESS(status)) {
    net_cleanup();
    return status;
  }

  ObReferenceObjectByHandle(hThreadUDP, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID *)&g_WorkerThreadUDP, NULL);
  ZwClose(hThreadUDP);
  DbgPrint("[SERVER] UDP Loaded!\n");

  return STATUS_SUCCESS;
}
#include <ksockapi.h>


/*
 * Тестирование IPv6 и scope_id в kernel-драйвере
 *
 * Среда тестирования:
 *   - Хост: Windows 10
 *   - VM: Windows (гостевая) с сетевым адаптером VMware VMnet1 (Host-Only)
 *   - Сервер: запущен на VM
 *   - Клиент: ncat на хосте
 *
 * Настройка сети для IPv6:
 *   1. На хосте добавлен ULA адрес на VMnet1 (индекс 6):
 *      netsh interface ipv6 add address 6 fd00:dead:beef:2::1
 *   2. На VM добавлен ULA адрес на Ethernet0 (индекс 9):
 *      netsh interface ipv6 add address 9 fd00:dead:beef:2::2
 *
 * Результаты тестирования:
 *   TCP через ULA адрес (без scope_id)      - работает
 *   TCP через link-local адрес (с scope_id) - работает
 *   UDP через ULA адрес                     - работает
 *   UDP через link-local адрес (с scope_id) - работает
 *   Корректное извлечение scope_id из адреса клиента
 *   Корректное отображение scope_id в выводе
 *
 * Вывод:
 *   Драйвер корректно обрабатывает IPv6 адреса с scope_id
 *   и без него. link-local адреса (fe80::/10) требуют
 *   обязательного указания scope_id для работы.
 */


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

  // Привязка для TCP
  /*
  addr.family = NET_AF_INET6;
  net_htons(PORT_TCP, &addr.port);
  addr.addr.ipv4 = 0;*/
  
  /*
  addr.family = NET_AF_INET6;
  net_htons(PORT_TCP, &addr.port);
  memset(addr.addr.ipv6, 0, 16); // :: - все IPv6 интерфейсы
  */

  err = net_address_parse("[fe80::a921:2d1f:61fb:8942%9]", NET_AF_INET6, &addr);
  net_htons(PORT_TCP, &addr.port);
  if (err != NET_SUCCESS) goto close_server;

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

      if (client_addr.family == NET_AF_INET6 && client_addr.scope_id != 0) {
        DbgPrint("[SERVER_TCP] [%s scope_id=%u] >> %s;\n", ip_str,
                 client_addr.scope_id, buffer);
      } else {
        DbgPrint("[SERVER_TCP] [%s] >> %s;\n", ip_str, buffer);
      }
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

  // Привязка для UDP
  /*
    addr.family = NET_AF_INET4;
    net_htons(PORT_UDP, &addr.port);
    addr.addr.ipv4 = 0;
  */

  /*
  addr.family = NET_AF_INET6;
  net_htons(PORT_UDP, &addr.port);
  memset(addr.addr.ipv6, 0, 16); // :: - все IPv6 интерфейсы
  */

  err = net_address_parse("[fe80::a921:2d1f:61fb:8942%9]", NET_AF_INET6, &addr);
  net_htons(PORT_UDP, &addr.port);
  if (err != NET_SUCCESS) goto close_socket;

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

      if (client_addr.family == NET_AF_INET6 && client_addr.scope_id != 0) {
        DbgPrint("[SERVER_UDP] [%s scope_id=%u] >> %s\n", ip_str,
                 client_addr.scope_id, buffer);
      } else {
        DbgPrint("[SERVER_UDP] [%s] >> %s\n", ip_str, buffer);
      }
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
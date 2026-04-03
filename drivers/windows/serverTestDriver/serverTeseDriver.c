#include <ntddk.h>
#include <wsk.h>
#include <ksockapi.h>
#include "../../../src/common/windows/windows_common.h"

#define PORT 4444
#define BUFFER_SIZE 512

WSK_REGISTRATION g_WskRegistration;
WSK_PROVIDER_NPI g_WskProviderNpi;

volatile BOOLEAN g_StopWorker = FALSE;

PETHREAD g_WorkerThread = NULL;

PWSK_SOCKET listenSocket;

const WSK_CLIENT_DISPATCH WskAppDispatch = {MAKE_WSK_VERSION(1, 0), 0, NULL};

WSK_CLIENT_NPI WskClientNpi = {NULL, &WskAppDispatch};

// ===== completion =====
NTSTATUS CompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                           PVOID Context) {
  UNREFERENCED_PARAMETER(DeviceObject);
  UNREFERENCED_PARAMETER(Irp);
  KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SyncIrp(PIRP irp, KEVENT *event, NTSTATUS status) {
  if (status == STATUS_PENDING) {
    KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);
    status = irp->IoStatus.Status;
  }
  return status;
}

// ===== worker =====
VOID NetworkWorkerThread(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

  NTSTATUS status;
  KEVENT event;
  PIRP irp;

  // ===== WSK init =====
  status = WskRegister(&WskClientNpi, &g_WskRegistration);
  if (!NT_SUCCESS(status))
    goto exit;

  status =
      WskCaptureProviderNPI(&g_WskRegistration, WSK_NO_WAIT, &g_WskProviderNpi);
  if (!NT_SUCCESS(status))
    goto exit;

  // ===== create listen socket =====
  irp = IoAllocateIrp(1, FALSE);
  KeInitializeEvent(&event, NotificationEvent, FALSE);
  IoSetCompletionRoutine(irp, CompletionRoutine, &event, TRUE, TRUE, TRUE);

status = g_WskProviderNpi.Dispatch->WskSocket(
      g_WskProviderNpi.Client, AF_INET, SOCK_STREAM, IPPROTO_TCP,
      WSK_FLAG_LISTEN_SOCKET,
      NULL,            // SocketContext
      &WskAppDispatch, // Dispatch
      NULL,            // OwningProcess
      NULL,            // OwningThread
      NULL,            // SecurityDescriptor 👈 ВАЖНО
      irp);

  status = SyncIrp(irp, &event, status);
  listenSocket = (PWSK_SOCKET)irp->IoStatus.Information;
  IoFreeIrp(irp);

  // ===== bind =====
  SOCKADDR_IN addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = RtlUshortByteSwap(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  irp = IoAllocateIrp(1, FALSE);
  KeInitializeEvent(&event, NotificationEvent, FALSE);
  IoSetCompletionRoutine(irp, CompletionRoutine, &event, TRUE, TRUE, TRUE);

  status = ((PWSK_PROVIDER_LISTEN_DISPATCH)listenSocket->Dispatch)
               ->WskBind(listenSocket, (PSOCKADDR)&addr, 0, irp);

  status = SyncIrp(irp, &event, status);
  IoFreeIrp(irp);

  DbgPrint("[test] listening on %d\n", PORT);

  // ===== accept =====
  irp = IoAllocateIrp(1, FALSE);
  KeInitializeEvent(&event, NotificationEvent, FALSE);
  IoSetCompletionRoutine(irp, CompletionRoutine, &event, TRUE, TRUE, TRUE);

  PWSK_SOCKET clientSocket;

  SOCKADDR_IN localAddr = {0};
  SOCKADDR_IN remoteAddr = {0};

  status = ((PWSK_PROVIDER_LISTEN_DISPATCH)listenSocket->Dispatch)
               ->WskAccept(listenSocket, 0,
                           NULL, // AcceptSocketContext
                           NULL, // AcceptDispatch
                           (PSOCKADDR)&localAddr, (PSOCKADDR)&remoteAddr, irp);

  status = SyncIrp(irp, &event, status);
  clientSocket = (PWSK_SOCKET)irp->IoStatus.Information;
  IoFreeIrp(irp);

  DbgPrint("[test] client connected\n");

  // ===== подключаем твою либу =====
  WINDOWS_SOCKET_IMPL impl = {0};
  impl.active_client = clientSocket;

  net_socket_t sock = {0};
  sock.context = &impl;
  sock.protocol = NET_PROTO_TCP;

  // ===== receive loop =====
  while (!g_StopWorker) {
    char buffer[BUFFER_SIZE];
    size_t received = 0;

    net_error_t err =
        net_socket_receive(&sock, buffer, BUFFER_SIZE, NULL, &received);

    if (err != NET_SUCCESS || received == 0) {
      DbgPrint("[test] disconnect or error (%d)\n", err);
      break;
    }

    DbgPrint("[test] worker exit\n");

    // ❗ гарантируем null-terminated строку
    if (received < BUFFER_SIZE) {
      buffer[received] = '\0';
    } else {
      buffer[BUFFER_SIZE - 1] = '\0';
    }

    DbgPrint("[recv] %s\n", buffer);
  }

exit:
  PsTerminateSystemThread(STATUS_SUCCESS);
}

// ===== unload =====
VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  g_StopWorker = TRUE;

  if (g_WorkerThread) {
    KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(g_WorkerThread);
  }

  WskReleaseProviderNPI(&g_WskRegistration);
  WskDeregister(&g_WskRegistration);
}

// ===== entry =====
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  HANDLE hThread;
  NTSTATUS status;

  status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                NetworkWorkerThread, NULL);

  if (!NT_SUCCESS(status))
    return status;

  ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType,
                            KernelMode, (PVOID *)&g_WorkerThread, NULL);

  ZwClose(hThread);

  return STATUS_SUCCESS;
}
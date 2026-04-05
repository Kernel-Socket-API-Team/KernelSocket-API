#include <ntddk.h>
#include <wdm.h>
#include <wsk.h>

typedef struct _WSK_APP_SOCKET_CONTEXT {
  PWSK_SOCKET Socket;
  PKEVENT CompletionEvent;
} WSK_APP_SOCKET_CONTEXT, *PWSK_APP_SOCKET_CONTEXT;

typedef struct _CONNECT_CONTEXT {
  PKEVENT CompletionEvent;
  NTSTATUS FinalStatus;
} CONNECT_CONTEXT, *PCONNECT_CONTEXT;

typedef struct _SEND_CONTEXT {
  PKEVENT CompletionEvent;
  PMDL Mdl;
  PVOID Buffer;
} SEND_CONTEXT, *PSEND_CONTEXT;

typedef struct _CLOSE_CONTEXT {
  PKEVENT CompletionEvent;
  NTSTATUS FinalStatus;
} CLOSE_CONTEXT, *PCLOSE_CONTEXT;

//создаем TCP сокет

NTSTATUS CreateTcpSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                                 PVOID Context) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PWSK_APP_SOCKET_CONTEXT SocketContext = (PWSK_APP_SOCKET_CONTEXT)Context;

  if (Irp->IoStatus.Status == STATUS_SUCCESS) {
    SocketContext->Socket = (PWSK_SOCKET)(Irp->IoStatus.Information);
  }

  KeSetEvent(SocketContext->CompletionEvent, IO_NO_INCREMENT, FALSE);
  IoFreeIrp(Irp);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS CreateTcpSocket(PWSK_PROVIDER_NPI WskProviderNpi,
                         PWSK_APP_SOCKET_CONTEXT SocketContext) {
  PIRP Irp;
  NTSTATUS Status;

  Irp = IoAllocateIrp(1, FALSE);
  if (!Irp)
    return STATUS_INSUFFICIENT_RESOURCES;

  IoSetCompletionRoutine(Irp, CreateTcpSocketComplete, SocketContext, TRUE,
                         TRUE, TRUE);

  Status = WskProviderNpi->Dispatch->WskSocket(
      WskProviderNpi->Client,
      AF_INET,                    
      SOCK_STREAM,                
      IPPROTO_TCP,                
      WSK_FLAG_CONNECTION_SOCKET, 
                                  
      SocketContext, NULL, NULL, NULL, NULL, Irp);

  return Status;
}

//Connect

NTSTATUS ConnectComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PCONNECT_CONTEXT ConnectContext = (PCONNECT_CONTEXT)Context;
  ConnectContext->FinalStatus = Irp->IoStatus.Status;

  KeSetEvent(ConnectContext->CompletionEvent, IO_NO_INCREMENT, FALSE);
  IoFreeIrp(Irp);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS ConnectSocket(PWSK_SOCKET Socket, PSOCKADDR RemoteAddress,
                       PCONNECT_CONTEXT ConnectContext) {
  PIRP Irp;
  NTSTATUS Status;

  // Для TCP используем CONNECTION dispatch
  PWSK_PROVIDER_CONNECTION_DISPATCH Dispatch =
      (PWSK_PROVIDER_CONNECTION_DISPATCH)(Socket->Dispatch);

  Irp = IoAllocateIrp(1, FALSE);
  if (!Irp)
    return STATUS_INSUFFICIENT_RESOURCES;

  IoSetCompletionRoutine(Irp, ConnectComplete, ConnectContext, TRUE, TRUE,
                         TRUE);

  // WskConnect — устанавливает соединение с сервером
  Status = Dispatch->WskConnect(Socket, RemoteAddress,
                                0, // Flags
                                Irp);

  return Status;
}

//Send для TCP

NTSTATUS SendTcpComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PSEND_CONTEXT SendContext = (PSEND_CONTEXT)Context;

  KeSetEvent(SendContext->CompletionEvent, IO_NO_INCREMENT, FALSE);

  if (SendContext->Mdl) {
    IoFreeMdl(SendContext->Mdl);
  }
  if (SendContext->Buffer) {
    ExFreePool(SendContext->Buffer);
  }

  ExFreePoolWithTag(SendContext, 'senC');
  IoFreeIrp(Irp);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SendTcp(PWSK_SOCKET Socket, PWSK_BUF Buffer,
                 PSEND_CONTEXT SendContext) {
  PIRP Irp;
  NTSTATUS Status;

  // Для TCP используем CONNECTION dispatch
  PWSK_PROVIDER_CONNECTION_DISPATCH Dispatch =
      (PWSK_PROVIDER_CONNECTION_DISPATCH)(Socket->Dispatch);

  Irp = IoAllocateIrp(1, FALSE);
  if (!Irp)
    return STATUS_INSUFFICIENT_RESOURCES;

  IoSetCompletionRoutine(Irp, SendTcpComplete, SendContext, TRUE, TRUE, TRUE);

  // WskSend
  Status = Dispatch->WskSend(Socket, Buffer,
                             0, // Flags
                             Irp);

  return Status;
}

//Close

NTSTATUS CloseSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                             PVOID Context) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PCLOSE_CONTEXT CloseContext = (PCLOSE_CONTEXT)Context;
  CloseContext->FinalStatus = Irp->IoStatus.Status;

  KeSetEvent(CloseContext->CompletionEvent, IO_NO_INCREMENT, FALSE);
  IoFreeIrp(Irp);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS CloseSocket(PWSK_SOCKET Socket, PCLOSE_CONTEXT CloseContext) {
  PIRP Irp;
  NTSTATUS Status;

  PWSK_PROVIDER_BASIC_DISPATCH Dispatch =
      (PWSK_PROVIDER_BASIC_DISPATCH)(Socket->Dispatch);

  Irp = IoAllocateIrp(1, FALSE);
  if (!Irp)
    return STATUS_INSUFFICIENT_RESOURCES;

  IoSetCompletionRoutine(Irp, CloseSocketComplete, CloseContext, TRUE, TRUE,
                         TRUE);

  Status = Dispatch->WskCloseSocket(Socket, Irp);

  return Status;
}

// Driver Entry

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  DbgPrint("TCP Client Driver unloaded!\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;
  DbgPrint("TCP Client Driver started!\n");

  // Регистрация WSK
  const WSK_CLIENT_DISPATCH WskAppDispatch = {MAKE_WSK_VERSION(1, 0), 0, NULL};
  WSK_REGISTRATION WskRegistration;
  WSK_CLIENT_NPI WskClientNpi;

  WskClientNpi.ClientContext = NULL;
  WskClientNpi.Dispatch = &WskAppDispatch;

  NTSTATUS Status = WskRegister(&WskClientNpi, &WskRegistration);
  if (!NT_SUCCESS(Status))
    return Status;

  // Capture Provider
  LARGE_INTEGER Timeout;
  Timeout.QuadPart = -5 * 1000 * 10000; // 5 секунд

  WSK_PROVIDER_NPI WskProviderNpi;
  Status = WskCaptureProviderNPI(&WskRegistration, (ULONG)Timeout.QuadPart,
                                 &WskProviderNpi);

  if (!NT_SUCCESS(Status)) {
    WskDeregister(&WskRegistration);
    return Status;
  }

  // События
  KEVENT SocketCreated, ConnectCompleted, SendCompleted, CloseCompleted;
  KeInitializeEvent(&SocketCreated, NotificationEvent, FALSE);
  KeInitializeEvent(&ConnectCompleted, NotificationEvent, FALSE);
  KeInitializeEvent(&SendCompleted, NotificationEvent, FALSE);
  KeInitializeEvent(&CloseCompleted, NotificationEvent, FALSE);

  // Создаем TCP сокет
  WSK_APP_SOCKET_CONTEXT SocketContext = {0};
  SocketContext.CompletionEvent = &SocketCreated;

  Status = CreateTcpSocket(&WskProviderNpi, &SocketContext);
  if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
    goto Cleanup;

  KeWaitForSingleObject(&SocketCreated, Executive, KernelMode, FALSE, NULL);

  if (!SocketContext.Socket) {
    DbgPrint("Failed to create TCP socket\n");
    goto Cleanup;
  }

  DbgPrint("TCP Socket created!\n");

  // Подключаемся к серверу (отличие от UDP)
  SOCKADDR_IN remoteAddr = {0};
  remoteAddr.sin_family = AF_INET;
  remoteAddr.sin_port = RtlUshortByteSwap(9003);

  const char *terminator = NULL;
  IN_ADDR ip;
  Status = RtlIpv4StringToAddressA("192.168.0.112", FALSE, &terminator, &ip);
  if (!NT_SUCCESS(Status))
    goto Cleanup;

  remoteAddr.sin_addr = ip;

  CONNECT_CONTEXT connectContext = {0};
  connectContext.CompletionEvent = &ConnectCompleted;

  Status = ConnectSocket(SocketContext.Socket, (PSOCKADDR)&remoteAddr,
                         &connectContext);

  if (Status == STATUS_PENDING) {
    KeWaitForSingleObject(&ConnectCompleted, Executive, KernelMode, FALSE,
                          NULL);
    Status = connectContext.FinalStatus;
  }

  if (!NT_SUCCESS(Status)) {
    DbgPrint("Failed to connect: 0x%X\n", Status);
    goto Cleanup;
  }

  DbgPrint("Connected to server!\n");

  // Отправляем данные
  const char *textSend = "Hello from TCP!";
  const int size = 15;

  char *msg = ExAllocatePoolWithTag(NonPagedPoolNx, size, 'msgT');
  if (!msg)
    goto Cleanup;

  RtlCopyMemory(msg, textSend, size);

  PMDL mdl = IoAllocateMdl(msg, size, FALSE, FALSE, NULL);
  if (!mdl) {
    ExFreePool(msg);
    goto Cleanup;
  }

  MmBuildMdlForNonPagedPool(mdl);

  PSEND_CONTEXT sendCtx =
      ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SEND_CONTEXT), 'senC');
  if (!sendCtx) {
    IoFreeMdl(mdl);
    ExFreePool(msg);
    goto Cleanup;
  }

  RtlZeroMemory(sendCtx, sizeof(SEND_CONTEXT));
  sendCtx->CompletionEvent = &SendCompleted;
  sendCtx->Mdl = mdl;
  sendCtx->Buffer = msg;

  WSK_BUF wskBuf;
  wskBuf.Mdl = mdl;
  wskBuf.Offset = 0;
  wskBuf.Length = size;

  Status = SendTcp(SocketContext.Socket, &wskBuf, sendCtx);

  if (NT_SUCCESS(Status) || Status == STATUS_PENDING) {
    KeWaitForSingleObject(&SendCompleted, Executive, KernelMode, FALSE, NULL);
    DbgPrint("Message sent via TCP!\n");
  }

Cleanup:
  if (SocketContext.Socket) {
    CLOSE_CONTEXT closeContext = {0};
    closeContext.CompletionEvent = &CloseCompleted;

    CloseSocket(SocketContext.Socket, &closeContext);
    KeWaitForSingleObject(&CloseCompleted, Executive, KernelMode, FALSE, NULL);
  }

  WskReleaseProviderNPI(&WskRegistration);
  WskDeregister(&WskRegistration);

  DbgPrint("TCP Client finished!\n");
  return STATUS_SUCCESS;
}
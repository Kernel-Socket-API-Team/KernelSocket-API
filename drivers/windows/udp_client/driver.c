#include <ntddk.h>
#include <wsk.h>
#include <wdm.h>

// Documentation: https://learn.microsoft.com/ru-ru/windows-hardware/drivers/network/registering-a-winsock-kernel-application

//Create Socket

// Socket Context
typedef struct _WSK_APP_SOCKET_CONTEXT {
  PWSK_SOCKET Socket;
  // ...
} WSK_APP_SOCKET_CONTEXT, *PWSK_APP_SOCKET_CONTEXT;

typedef struct _SEND_CONTEXT {
  PWSK_BUF DatagramBuffer; // Буфер с данными
  PKEVENT CompletionEvent; // Событие для 
  // ...
} SEND_CONTEXT, *PSEND_CONTEXT;

// Prototype Socket Complete (функция заврешения)
NTSTATUS CreateListeningSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

// Создание сокета
NTSTATUS CreateListeningSocket(PWSK_PROVIDER_NPI WskProviderNpi, PWSK_APP_SOCKET_CONTEXT SocketContext, PWSK_CLIENT_LISTEN_DISPATCH Dispatch) {
  UNREFERENCED_PARAMETER(Dispatch);

  PIRP Irp;
  NTSTATUS Status;

  // Выделяем память под IRP
  Irp = IoAllocateIrp(1, FALSE);
  if (!Irp)
    return STATUS_INSUFFICIENT_RESOURCES;

  // Привязка IRP к завершающей функции, последние три TRUE:
  // TRUE (InvokeOnSuccess) — вызови при успехе
  // TRUE (InvokeOnError) — вызови при ошибке
  // TRUE (InvokeOnCancel) — вызови, если операцию отменили
  IoSetCompletionRoutine(Irp, CreateListeningSocketComplete, SocketContext, TRUE, TRUE, TRUE);

  // Создание сокета используя функцию из диспетчера
  // WskProviderNpi->Client - идентифицицрует драйвер в системе по клиенту
  // AF_INET - семейство адресов (в данном случае IPv4)
  // SOCK_DGRAM - тип сокета, в данном случае udp
  // IPPROTO_UDP - протокол транспортного уровня, в данном случае udp
  // WSK_FLAG_DATAGRAM_SOCKET - тип создаваемого сокета, в данном случае для UDP
  // SocketContext - указатель на контекст
  // NULL - указатель на таблицу функций обратного вызова, в данном случае они не нужны
  // NULL - первый NULL указывает на процесс которому будет принадлежать сокет, в нашем случае сокет будет принадлежать системе
  // NULL - второй NULL указатель на поток
  // NULL - третий NULL дескриптор безопастности
  // Irp - укзатель на ранее созданный IRP
  Status = WskProviderNpi->Dispatch->WskSocket(WskProviderNpi->Client, AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSK_FLAG_DATAGRAM_SOCKET, SocketContext, NULL, NULL, NULL, NULL, Irp);

  return Status;
}

NTSTATUS CreateListeningSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
    // Так как DeviceObject не используется, указываем, что девайс который завершил работу отсутсвует
  UNREFERENCED_PARAMETER(DeviceObject);

  PWSK_APP_SOCKET_CONTEXT SocketContext;

  // Проверяем удалось ли создать сокет
  if (Irp->IoStatus.Status == STATUS_SUCCESS) {
     
    // Задаем контекст
    SocketContext = (PWSK_APP_SOCKET_CONTEXT)Context;

    // Сохраняем созданный сокет в контекст
    SocketContext->Socket = (PWSK_SOCKET)(Irp->IoStatus.Information);
  
  } 

  // Освобождаем IRP
  IoFreeIrp(Irp);

  // Возвращаем всегда, чтобы завершить обработку IRP завершения
  return STATUS_MORE_PROCESSING_REQUIRED;
}

// Функция завершения отправки датаграммы для UDP
NTSTATUS SendDatagramComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

NTSTATUS SendDatagram(PWSK_SOCKET Socket, PWSK_BUF DatagramBuffer,
                      PSOCKADDR RemoteAddress, PSEND_CONTEXT SendContext) {
  NTSTATUS Status;
  PWSK_PROVIDER_DATAGRAM_DISPATCH Dispatch;
  PIRP Irp;

  Dispatch = (PWSK_PROVIDER_DATAGRAM_DISPATCH)(Socket->Dispatch);

  Irp = IoAllocateIrp(1, FALSE);

  if (!Irp)
    return STATUS_INSUFFICIENT_RESOURCES;

  IoSetCompletionRoutine(Irp, SendDatagramComplete, SendContext, TRUE, TRUE,
                         TRUE);

  Status = Dispatch->WskSendTo(Socket, DatagramBuffer, 0, RemoteAddress, 0, NULL, Irp);

  return Status;
}

NTSTATUS SendDatagramComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
  UNREFERENCED_PARAMETER(DeviceObject);
  UNREFERENCED_PARAMETER(Context);

  PWSK_BUF DatagramBuffer;
  ULONG ByteCount;
  PSEND_CONTEXT sendContext = (PSEND_CONTEXT)Context;

  if (Irp->IoStatus.Status == STATUS_SUCCESS) {
    DatagramBuffer = (PWSK_BUF)Context;
    ByteCount = (ULONG)(Irp->IoStatus.Information);  
  }

  KeSetEvent(sendContext->CompletionEvent, IO_NO_INCREMENT, FALSE);

  IoFreeIrp(Irp);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

// Функция завершения закрытия сокета
NTSTATUS CloseSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                             PVOID Context) {
  UNREFERENCED_PARAMETER(DeviceObject);
  UNREFERENCED_PARAMETER(Context);

  if (Irp->IoStatus.Status == STATUS_SUCCESS) {
    DbgPrint("Socket successfully closed\n");
    // Здесь можно освободить сам контекст, если он больше не нужен
    // ExFreePoolWithTag(SocketContext, 'tag');
  } else {
    DbgPrint("Failed to close socket: 0x%X\n", Irp->IoStatus.Status);
  }

  IoFreeIrp(Irp);
  return STATUS_MORE_PROCESSING_REQUIRED;
}

// Функция инициации закрытия сокета
NTSTATUS CloseSocket(PWSK_SOCKET Socket,
                     PWSK_APP_SOCKET_CONTEXT SocketContext) {
  PWSK_PROVIDER_BASIC_DISPATCH Dispatch;
  PIRP Irp;
  NTSTATUS Status;

  // 1. Получаем диспатчер (WSK_PROVIDER_BASIC_DISPATCH есть у всех сокетов)
  Dispatch = (PWSK_PROVIDER_BASIC_DISPATCH)(Socket->Dispatch);

  // 2. Создаём IRP
  Irp = IoAllocateIrp(1, FALSE);
  if (!Irp) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  // 3. Устанавливаем completion routine
  IoSetCompletionRoutine(Irp, CloseSocketComplete, SocketContext, TRUE, TRUE,
                         TRUE);

  // 4. Вызываем WskCloseSocket
  Status = Dispatch->WskCloseSocket(Socket, Irp);

  if (Status == STATUS_PENDING) {
    DbgPrint("Close operation pending...\n");
    // Здесь можно вернуть STATUS_PENDING и обработать завершение в колбэке,
    // либо подождать синхронно, используя KEVENT.
  } else if (!NT_SUCCESS(Status)) {
    // Если сразу ошибка — освобождаем IRP
    IoFreeIrp(Irp);
  }

  return Status;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {

  DbgPrint("Successful start driver!\n");
  UNREFERENCED_PARAMETER(DriverObject);
  UNREFERENCED_PARAMETER(RegistryPath);

  // Указываем версию WSK для клиента
  const WSK_CLIENT_DISPATCH WskAppDispatch = {MAKE_WSK_VERSION(1, 0), 0, NULL};

  // Регистрируем NPI клиента
  WSK_REGISTRATION WskRegistration;
  WSK_CLIENT_NPI WskClientNpi;

  WskClientNpi.ClientContext = NULL;
  WskClientNpi.Dispatch = &WskAppDispatch;
  NTSTATUS Status = WskRegister(&WskClientNpi, &WskRegistration);

  if (!NT_SUCCESS(Status))
    return Status;

  DbgPrint("Successful registration\n");

  /*
  Прямая цитата из документации:
  Важно, чтобы избежать негативного влияния на начало других драйверов и служб,
  приложение WSK, которое вызывает WskCaptureProviderNPI из функции DriverEntry,
  не должно задавать параметр WaitTimeout WSK_INFINITE_WAIT или чрезмерное
  время ожидания.
  */

  // - Означает относительное время (значит от текущего момента)
  // Без него конкретная дата (абсолютное время)
  // При этом система считает время в наносекундах

  LARGE_INTEGER Timeout;
  Timeout.QuadPart = -5 * 1000 * 10000; // Ждем 5 секунд

  // Запись NPI услуг поставщика
  WSK_PROVIDER_NPI WskProviderNpi;
  Status = WskCaptureProviderNPI(&WskRegistration, (ULONG)Timeout.QuadPart,
                                 &WskProviderNpi);

  if (!NT_SUCCESS(Status)) {
    WskDeregister(&WskRegistration);
    return Status;
  }

  DbgPrint("Successful NPI\n");

  KEVENT SocketCreated;
  KEVENT SendCompleted;

  KeInitializeEvent(&SocketCreated, NotificationEvent, FALSE);
  KeInitializeEvent(&SendCompleted, NotificationEvent, FALSE);

  WSK_APP_SOCKET_CONTEXT WskSocketContext = {0};

  Status = CreateListeningSocket(&WskProviderNpi, &WskSocketContext, NULL);
  if (NT_SUCCESS(Status)) {
    KeWaitForSingleObject(&SocketCreated, Executive, KernelMode, FALSE, NULL);

    if (WskSocketContext.Socket) {
      char msg[] = "Hello world!";
      PMDL mdl = IoAllocateMdl(msg, sizeof(msg), FALSE, FALSE, NULL);

      if (mdl) {
        MmBuildMdlForNonPagedPool(mdl);
        WSK_BUF buf;
        buf.Mdl = mdl;
        buf.Offset = 0;
        buf.Length = sizeof(msg);
        

        SOCKADDR_IN addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = 9003;
        RtlIpv4StringToAddressA("192.168.0.112", FALSE, NULL, &addr.sin_addr);

        SEND_CONTEXT sendCtx = {0};
        sendCtx.CompletionEvent = &SendCompleted;
        sendCtx.DatagramBuffer = &buf;

        Status = SendDatagram(WskSocketContext.Socket, &buf, (PSOCKADDR)&addr,
                              &sendCtx);
        if (NT_SUCCESS(Status)) {
          KeWaitForSingleObject(&SendCompleted, Executive, KernelMode, FALSE, NULL);
        }
        
        IoFreeMdl(mdl);
      }
    
    }
  }

  if (WskSocketContext.Socket) {
    Status = CloseSocket(WskSocketContext.Socket, &WskSocketContext);
  }

  // Закрываем NPI соединение
  WskReleaseProviderNPI(&WskRegistration);

  // Закрываем регистрацию пользователя
  WskDeregister(&WskRegistration);

  return Status;
}


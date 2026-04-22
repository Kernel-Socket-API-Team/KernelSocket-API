#include <ntddk.h>
#include <wdm.h>
#include <wsk.h>

// Documentation:
// https://learn.microsoft.com/ru-ru/windows-hardware/drivers/network/registering-a-winsock-kernel-application

// Socket Context
typedef struct _WSK_APP_SOCKET_CONTEXT
{
    PWSK_SOCKET Socket;      // Указатель на сокет
    PKEVENT CompletionEvent; // Событие успешного создания сокета
} WSK_APP_SOCKET_CONTEXT, *PWSK_APP_SOCKET_CONTEXT;

// Контекст отправки сообщения
typedef struct _SEND_CONTEXT
{
    WSK_BUF DatagramBuffer;  // Буфер с данными
    PKEVENT CompletionEvent; // Событие для успешной отправки датаграммы
    PMDL Mdl;                // Дескриптор области памяти где храним данные
    PVOID Buffer;            // Указатель на буфер с сообщением
} SEND_CONTEXT, *PSEND_CONTEXT;

// Структура контекста для Bind операции (явная привязка сокета)
typedef struct _BIND_CONTEXT
{
    PKEVENT CompletionEvent; // Событие для успешной привязки сокета
    NTSTATUS FinalStatus;    // Статус привязки
} BIND_CONTEXT, *PBIND_CONTEXT;

// Контекст для корретного закрытия сокета
typedef struct _CLOSE_CONTEXT
{
    PKEVENT CompletionEvent; // Событие для успешного закрытия сокета
    NTSTATUS FinalStatus;    // Статус закрытия
} CLOSE_CONTEXT, *PCLOSE_CONTEXT;

// Prototype Socket Complete (функция заврешения)
NTSTATUS CreateListeningSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

// Создание сокета
NTSTATUS CreateListeningSocket(PWSK_PROVIDER_NPI WskProviderNpi, PWSK_APP_SOCKET_CONTEXT SocketContext,
                               PWSK_CLIENT_LISTEN_DISPATCH Dispatch)
{
    UNREFERENCED_PARAMETER(Dispatch);

    PIRP Irp;
    NTSTATUS Status;

    // Выделяем память под IRP
    Irp = IoAllocateIrp(1, FALSE);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Привязка IRP к завершающей функции
    IoSetCompletionRoutine(Irp, CreateListeningSocketComplete, SocketContext, TRUE, TRUE, TRUE);

    // Создание сокета используя функцию из диспетчера
    Status = WskProviderNpi->Dispatch->WskSocket(WskProviderNpi->Client, AF_INET, SOCK_DGRAM, IPPROTO_UDP,
                                                 WSK_FLAG_DATAGRAM_SOCKET, SocketContext, NULL, NULL, NULL, NULL, Irp);

    return Status;
}

NTSTATUS CreateListeningSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    // Указываем что DeviceObject не используется в данной функции
    UNREFERENCED_PARAMETER(DeviceObject);

    // Задаем контекст
    PWSK_APP_SOCKET_CONTEXT SocketContext = (PWSK_APP_SOCKET_CONTEXT)Context;

    // Проверяем удалось ли создать сокет
    if (Irp->IoStatus.Status == STATUS_SUCCESS)
    {
        // Сохраняем созданный сокет в контекст
        SocketContext->Socket = (PWSK_SOCKET)(Irp->IoStatus.Information);
    }

    // Создаем событие о том что сокет успешно создан
    KeSetEvent(SocketContext->CompletionEvent, IO_NO_INCREMENT, FALSE);

    // Освобождаем IRP
    IoFreeIrp(Irp);

    // Возвращаем всегда, чтобы завершить обработку IRP завершения
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Функция завершения отправки датаграммы для UDP
NTSTATUS SendDatagramComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

NTSTATUS SendDatagram(PWSK_SOCKET Socket, PWSK_BUF DatagramBuffer, PSOCKADDR RemoteAddress, PSEND_CONTEXT SendContext)
{

    NTSTATUS Status;
    PWSK_PROVIDER_DATAGRAM_DISPATCH Dispatch;
    PIRP Irp;

    // Достаем диспетчер функций из сокета
    Dispatch = (PWSK_PROVIDER_DATAGRAM_DISPATCH)(Socket->Dispatch);

    // Выделяем память под Irp
    Irp = IoAllocateIrp(1, FALSE);

    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Привязываем Irp к завершающей функции
    IoSetCompletionRoutine(Irp, SendDatagramComplete, SendContext, TRUE, TRUE, TRUE);

    // Отправляем датаграмму используя функцию из диспетчера
    Status = Dispatch->WskSendTo(Socket, DatagramBuffer, 0, RemoteAddress, 0, NULL, Irp);

    return Status;
}

NTSTATUS SendDatagramComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    // Достаем контекст
    PSEND_CONTEXT sendContext = (PSEND_CONTEXT)Context;

    // Создаем событие о том что сообщение был успешно отправлено
    KeSetEvent(sendContext->CompletionEvent, IO_NO_INCREMENT, FALSE);

    // Освобождаем MDL
    if (sendContext->Mdl)
    {
        IoFreeMdl(sendContext->Mdl);
        sendContext->Mdl = NULL;
    }

    // Освобождаем буфер с данными
    if (sendContext->Buffer)
        ExFreePool(sendContext->Buffer);

    // Освобождаем память, которая была выделена для контекста в DriverEntry (избегаем хранение на стеке)
    ExFreePoolWithTag(sendContext, 'senC');

    // Освобождаем Irp
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Completion routine для Bind
NTSTATUS BindComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

// Функция для синхронной привязки сокета
NTSTATUS BindSocket(PWSK_SOCKET Socket, PWSK_PROVIDER_DATAGRAM_DISPATCH Dispatch, PVOID Context)
{

    NTSTATUS status;
    PBIND_CONTEXT bindCtx = (PBIND_CONTEXT)Context;
    PIRP bindIrp;

    // Подготавливаем локальный адрес (wildcard)
    SOCKADDR_IN localAddr = {0};
    localAddr.sin_family = AF_INET;         // IPv4
    localAddr.sin_addr.s_addr = INADDR_ANY; // любой интерфейс
    localAddr.sin_port = 0;                 // любой свободный порт

    // Создаем IRP
    bindIrp = IoAllocateIrp(1, FALSE);
    if (!bindIrp)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Подготавливаем контекст
    bindCtx->FinalStatus = STATUS_UNSUCCESSFUL;

    // Устанавливаем completion routine
    IoSetCompletionRoutine(bindIrp, BindComplete, bindCtx, TRUE, TRUE, TRUE);

    // Вызываем WskBind из диспетчера функций
    status = Dispatch->WskBind(Socket, (PSOCKADDR)&localAddr, 0, bindIrp);

    // Если операция асинхронная - ждем
    if (status == STATUS_PENDING)
        status = bindCtx->FinalStatus;

    return status;
}

// Completion routine для Bind
NTSTATUS BindComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PBIND_CONTEXT bindCtx = (PBIND_CONTEXT)Context;

    // Сохраняем результат операции
    bindCtx->FinalStatus = Irp->IoStatus.Status;

    // Будим ожидающий поток
    KeSetEvent(bindCtx->CompletionEvent, IO_NO_INCREMENT, FALSE);

    // Освобождаем Irp
    IoFreeIrp(Irp);

    // IRP будет освобожден вызывающей стороной после ожидания
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Функция завершения закрытия сокета
NTSTATUS CloseSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

// Функция инициации закрытия сокета
NTSTATUS CloseSocket(PWSK_SOCKET Socket, PVOID Context)
{

    PWSK_PROVIDER_BASIC_DISPATCH Dispatch;
    PIRP Irp;
    NTSTATUS Status;
    PCLOSE_CONTEXT CloseContext = (PCLOSE_CONTEXT)Context;

    // Получаем диспатчер (WSK_PROVIDER_BASIC_DISPATCH есть у всех сокетов)
    Dispatch = (PWSK_PROVIDER_BASIC_DISPATCH)(Socket->Dispatch);

    // Создаём IRP
    Irp = IoAllocateIrp(1, FALSE);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Устанавливаем completion routine
    IoSetCompletionRoutine(Irp, CloseSocketComplete, CloseContext, TRUE, TRUE, TRUE);

    // Вызываем WskCloseSocket
    Status = Dispatch->WskCloseSocket(Socket, Irp);

    return Status;
}

NTSTATUS CloseSocketComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    // Достаем контекст и устанавливаем нужные значения
    PCLOSE_CONTEXT CloseContext = (PCLOSE_CONTEXT)Context;
    CloseContext->FinalStatus = Irp->IoStatus.Status;

    // Вызываем событие завершения закрытия
    KeSetEvent(CloseContext->CompletionEvent, IO_NO_INCREMENT, FALSE);

    // Освобождаем Irp
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{

    UNREFERENCED_PARAMETER(RegistryPath);

    // Назначаем функцию для выгрузки драйвера, для того чтобы можно было воспользоваться sc.exe stop ...
    DriverObject->DriverUnload = DriverUnload;

    DbgPrint("Successful start driver!\n");

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
    Status = WskCaptureProviderNPI(&WskRegistration, (ULONG)Timeout.QuadPart, &WskProviderNpi);

    if (!NT_SUCCESS(Status))
    {
        WskDeregister(&WskRegistration);
        return Status;
    }

    // Инициализируем события для синхронной работы в DriverEntry
    KEVENT SocketCreated;
    KEVENT SendCompleted;
    KEVENT BindCompleted;
    KEVENT CloseCompleted;

    KeInitializeEvent(&SocketCreated, NotificationEvent, FALSE);
    KeInitializeEvent(&SendCompleted, NotificationEvent, FALSE);
    KeInitializeEvent(&BindCompleted, NotificationEvent, FALSE);
    KeInitializeEvent(&CloseCompleted, NotificationEvent, FALSE);

    WSK_APP_SOCKET_CONTEXT WskSocketContext = {0};
    WskSocketContext.CompletionEvent = &SocketCreated;

    // Создаем сокет
    Status = CreateListeningSocket(&WskProviderNpi, &WskSocketContext, NULL);
    if (NT_SUCCESS(Status))
    {

        KeWaitForSingleObject(&SocketCreated, Executive, KernelMode, FALSE, NULL);
        // Получаем диспетчер созданного сокета
        PWSK_PROVIDER_DATAGRAM_DISPATCH dispatch = (PWSK_PROVIDER_DATAGRAM_DISPATCH)WskSocketContext.Socket->Dispatch;

        // Явно привязываем сокет
        /*
        Вообще документация гвоорит что сокет должен привязываться автоматически,
        но автоматическая привязка в данном случае по неизвестным причинам просто не
        работает. Поэтому явно привязываем созданный сокет, документация этого не запрещает.
        В данном контексте привязать означает указать локального отправителя через любой доступный
        интерфес и соотвесвенно порт.
        */

        BIND_CONTEXT bindContext = {0};
        bindContext.CompletionEvent = &BindCompleted;

        NTSTATUS bindStatus = BindSocket(WskSocketContext.Socket, dispatch, &bindContext);

        if (!NT_SUCCESS(bindStatus))
        {
            /*
            Оператор перехода, который используется для прыжка вперед
            чтобы корретно очистить ресурсы которые были выделены для работы драйвера.
            */
            goto Cleanup;
        }

        KeWaitForSingleObject(&BindCompleted, Executive, KernelMode, FALSE, NULL);

        if (WskSocketContext.Socket)
        {
            char* msg = ExAllocatePoolWithTag(NonPagedPoolNx, 32, 'msgT');

            const char* textSend = "Hello world!";
            const int size = 12;

            RtlCopyMemory(msg, textSend, size);

            if (!msg)
                goto Cleanup;

            PMDL mdl = IoAllocateMdl(msg, size, FALSE, FALSE, NULL);

            if (mdl)
            {
                MmBuildMdlForNonPagedPool(mdl);

                // Заполняем данный куда хотим отправить сообщение
                SOCKADDR_IN addr = {0};
                addr.sin_family = AF_INET;
                addr.sin_port = RtlUshortByteSwap(9003);

                IN_ADDR ip;

                // Переводим адрес в двоичное представление
                const char* terminator = NULL; // Очень важно использовать terminator именно так!
                Status = RtlIpv4StringToAddressA("192.168.0.112", FALSE, &terminator, &ip);

                if (!NT_SUCCESS(Status))
                    goto Cleanup;

                addr.sin_addr = ip;

                // Выделяем память в невыгружаемом пуле
                PSEND_CONTEXT sendCtx =
                    (PSEND_CONTEXT)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SEND_CONTEXT), 'senC');

                RtlZeroMemory(sendCtx, sizeof(SEND_CONTEXT));
                sendCtx->CompletionEvent = &SendCompleted;
                sendCtx->Mdl = mdl;
                sendCtx->Buffer = msg;

                sendCtx->DatagramBuffer.Mdl = mdl;
                sendCtx->DatagramBuffer.Offset = 0;
                sendCtx->DatagramBuffer.Length = size;

                Status = SendDatagram(WskSocketContext.Socket, &sendCtx->DatagramBuffer, (PSOCKADDR)&addr, sendCtx);

                if (NT_SUCCESS(Status) || Status == STATUS_PENDING)
                {
                    KeWaitForSingleObject(&SendCompleted, Executive, KernelMode, FALSE, NULL);
                    DbgPrint("Successful send datagram\n");
                }
            }
        }
    }

// Метка для очистки
Cleanup:

    if (WskSocketContext.Socket)
    {
        CLOSE_CONTEXT CloseContext = {0};
        CloseContext.CompletionEvent = &CloseCompleted;

        Status = CloseSocket(WskSocketContext.Socket, &CloseContext);
        KeWaitForSingleObject(&CloseCompleted, Executive, KernelMode, FALSE, NULL);
    }

    // Закрываем NPI соединение
    WskReleaseProviderNPI(&WskRegistration);

    // Закрываем регистрацию пользователя
    WskDeregister(&WskRegistration);

    DbgPrint("Successful work end!\n");

    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrint("Driver unloaded!\n");
}
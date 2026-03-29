#include <ntddk.h>
#include <ksockapi.h>

// Отслеживаем рабочий поток
static HANDLE g_WorkerHandle = NULL;

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  DbgPrint("Driver unloading...");

  // Ждем завершения рабочего потока
  if (g_WorkerHandle) {
    KeWaitForSingleObject(g_WorkerHandle, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(g_WorkerHandle);
    ZwClose(g_WorkerHandle);
    g_WorkerHandle = NULL;
    DbgPrint("Thread free successfuly!");
  }

  // Очищаем сетевые ресурсы
  net_cleanup();

  DbgPrint("Status after net_cleanup = %d", net_is_ready());

  DbgPrint("Driver unloaded");
}

VOID NetworkWorkerThread(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

    if (net_is_ready())
        DbgPrint("net_is_ready() completed!");
    else {
      net_error_t err = net_activate(15000); // 15 секунд

      if (err == NET_ERROR_TIMEOUT)
        DbgPrint("Timeout!");
      else if (err != NET_SUCCESS)
        DbgPrint("Error = %d", err);

      // Пытаемся еще раз активироваться
      err = net_activate(15000);
      DbgPrint("net_activate() one more = %d", err);

      DbgPrint("Status after net_activate = %d", net_is_ready());
    }

    // Здесь происходит работа с сокетами
    
    // ...

    PsTerminateSystemThread(STATUS_SUCCESS);
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  DbgPrint("Start status = %d", net_is_ready());

  // Негативные тесты:

  net_error_t err = net_cleanup();
  DbgPrint("Try net_cleanup = %d", err);

  err = net_activate(15000);
  DbgPrint("Try net_activate = %d", err);

  err = net_is_ready();

  DbgPrint("Status lib = %d", err);

  // Окончание негативных сценариев

  DbgPrint("Test driver load!");

  // Вызываем общую инициализацию
  err = net_register();
  if (err != NET_SUCCESS) {
    DbgPrint("net_register() failed: %d", err);
    return STATUS_UNSUCCESSFUL;
  }

  DbgPrint("net_register() completed, creating worker thread...");

  // Пробуем вызвать net_register еще раз
  err = net_register();
  DbgPrint("net_register() one more = %d", err);

  // Пробуем очистить не до конца инициализированную библиотеку (ожидается успех)
  err = net_cleanup();
  DbgPrint("Try net_cleanup() without net_activate(...) = %d", err);

  // Заново регестрируемся
  err = net_register();
  DbgPrint("net_register() one more after net_cleanup = %d", err);

  DbgPrint("Status after net_register = %d", net_is_ready());

  // Пример создания рабочего птока
  NTSTATUS Status = PsCreateSystemThread(&g_WorkerHandle, 0, NULL, NULL, NULL, NetworkWorkerThread, NULL);

  ObReferenceObjectByHandle(g_WorkerHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, (PVOID *)&g_WorkerHandle, NULL);

  if (!NT_SUCCESS(Status)) {
    net_cleanup();
    DbgPrint("Failed to create worker thread!");
    return Status;
  }

  DbgPrint("Worker thread created, DriverEntry returning...");

  return STATUS_SUCCESS;
}
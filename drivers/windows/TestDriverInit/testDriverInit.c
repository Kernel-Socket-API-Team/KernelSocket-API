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
    ZwClose(g_WorkerHandle);
    g_WorkerHandle = NULL;
  }

  // Очищаем сетевые ресурсы
  net_cleanup();

  DbgPrint("Driver unloaded");
}

VOID NetworkWorkerThread(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

    if (net_is_ready())
        DbgPrint("net_is_ready() completed!");
    else {
      net_error_t err = net_wait_ready(15000); // 15 секунд

      if (err == NET_ERROR_TIMEOUT)
        DbgPrint("Timeout!");
      else if (err != NET_SUCCESS)
        DbgPrint("Error = %d", err);
    }

    // Здесь происходит работа с сокетами
    
    // ...

    PsTerminateSystemThread(STATUS_SUCCESS);
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  DbgPrint("Test driver load!");

  // Вызываем общую инициализацию
  net_error_t err = net_initialize();
  if (err != NET_SUCCESS) {
    DbgPrint("net_initialize() failed: %d", err);
    return STATUS_UNSUCCESSFUL;
  }

  DbgPrint("net_initialize() completed, creating worker thread...");

  // Пример создания рабочего птока
  NTSTATUS Status = PsCreateSystemThread(&g_WorkerHandle, 0, NULL, NULL, NULL, NetworkWorkerThread, NULL);

  if (!NT_SUCCESS(Status)) {
    net_cleanup();
    DbgPrint("Failed to create worker thread!");
    return Status;
  }

  DbgPrint("Worker thread created, DriverEntry returning...");

  return STATUS_SUCCESS;
}
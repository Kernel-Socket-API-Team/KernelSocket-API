#include <ksockapi.h>

// HANDLE потока
static PETHREAD g_WorkerThread = NULL;

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  if (g_WorkerThread) {
    KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(g_WorkerThread);
    g_WorkerThread = NULL;
  }

  net_cleanup();

  DbgPrint("Driver unloaded\n");
}

VOID NetworkWorkerThread(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

  net_error_t err = net_is_ready();

  if (err != NET_SUCCESS) 
      err = net_activate(NET_WAIT_INFINITE); // 15 секунд
 
  if (err == NET_SUCCESS) {
    // Работа с сокетами
    // ...
  } else if (err == NET_ERROR_TIMEOUT) {
    // обработка таймаута
    // ...
  }

  PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  NTSTATUS Status;
  net_error_t err;

  err = net_register();
  if (err != NET_SUCCESS) {
    return STATUS_UNSUCCESSFUL;
  }

  HANDLE hThread;

  Status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                NetworkWorkerThread, NULL);

  if (!NT_SUCCESS(Status)) {
    net_cleanup();
    return Status;
  }

  Status = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType,
                                KernelMode, (PVOID *)&g_WorkerThread, NULL);

  ZwClose(hThread);

  if (!NT_SUCCESS(Status)) {
    net_cleanup();
    return Status;
  }

  return STATUS_SUCCESS;
}
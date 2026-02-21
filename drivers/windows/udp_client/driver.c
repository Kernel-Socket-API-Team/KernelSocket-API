#include <ntddk.h>
#include <wsk.h>

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  KdPrint(("Minimal Driver: Unloaded\n"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  KdPrint(("Minimal Driver: Loaded!\n"));

  return STATUS_SUCCESS;
}
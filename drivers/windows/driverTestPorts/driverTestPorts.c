#include <ksockapi.h>

void DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  DbgPrint("Test driver unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  uint16_t test_value = 1234;
  uint16_t result;
  uint16_t back;
  net_error_t err;

  DriverObject->DriverUnload = DriverUnload;

  DbgPrint("=== Network Byte Order Test Driver ===\n");

  // Тест net_htons
  err = net_htons(test_value, &result);
  if (err != NET_SUCCESS) {
    DbgPrint("net_htons failed: %d\n", err);
    return STATUS_UNSUCCESSFUL;
  }

  DbgPrint("htons: 0x%04X -> 0x%04X\n", test_value, result);

  // Тест net_ntohs
  err = net_ntohs(result, &back);
  if (err != NET_SUCCESS) {
    DbgPrint("net_ntohs failed: %d\n", err);
    return STATUS_UNSUCCESSFUL;
  }

  DbgPrint("ntohs: 0x%04X -> 0x%04X\n", result, back);

  // Проверка
  if (back == test_value) {
    DbgPrint("TEST PASSED: 0x%04X -> 0x%04X -> 0x%04X\n", test_value, result,
             back);
  } else {
    DbgPrint("TEST FAILED: expected 0x%04X, got 0x%04X\n", test_value, back);
    return STATUS_UNSUCCESSFUL;
  }

  DbgPrint("=== Test completed ===\n");

  return STATUS_SUCCESS;
}
#include <ksockapi.h>

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  DbgPrint("[Test] Driver unloaded\n");
}

static void print_ipv6(const uint8_t *ipv6, char *buffer, size_t buffer_size) {
  RtlStringCbPrintfA(
      buffer, buffer_size,
      "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
      ipv6[0], ipv6[1], ipv6[2], ipv6[3], ipv6[4], ipv6[5], ipv6[6], ipv6[7],
      ipv6[8], ipv6[9], ipv6[10], ipv6[11], ipv6[12], ipv6[13], ipv6[14],
      ipv6[15]);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  DbgPrint("[Test] === Testing net_address_parse with scope_id ===\n");

  net_address_t addr;
  net_error_t err;

  // Тест IPv4 (scope_id должен игнорироваться)
  DbgPrint("\n[Test 1] IPv4 with scope (should ignore %%)\n");
  err = net_address_parse("192.168.1.100%ens33", NET_AF_INET4, &addr);
  DbgPrint("  Parse result: %d (expected -3)\n", err);
  DbgPrint("  IPv4 address: 0x%08X (expected 0x6401A8C0)\n", addr.addr.ipv4);
  DbgPrint("  scope_id: %u (expected 0)\n", addr.scope_id);

  // Тест IPv6 без scope_id
  DbgPrint("\n[Test 2] IPv6 without scope_id\n");
  err = net_address_parse("[fe80::1]", NET_AF_INET6, &addr);
  DbgPrint("  Parse result: %d (expected -3)\n", err);
  DbgPrint("  scope_id: %u (expected 0)\n", addr.scope_id);

  // Проверка, что адрес правильный (fe80::1)
  uint8_t expected_ipv6[] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  int match = 1;
  for (int i = 0; i < 16; i++) {
    if (addr.addr.ipv6[i] != expected_ipv6[i]) {
      match = 0;
      break;
    }
  }
  DbgPrint("  Address correct: %s\n", match ? "YES" : "NO");
  // Тест IPv6 с scope_id (число)
  DbgPrint("\n[Test 3] IPv6 with numeric scope_id (%%13)\n");
  err = net_address_parse("[fe80::1%13]", NET_AF_INET6, &addr);
  DbgPrint("  Parse result: %d (expected -3)\n", err);
  DbgPrint("  scope_id: %u (expected 13)\n", addr.scope_id);
  DbgPrint("  Success: %s\n", (addr.scope_id == 13) ? "YES" : "NO");

  // Тест IPv6 с scope_id (имя интерфейса которого нету в системе, соотвесвенно scope_id не запишется и останется старым)
  DbgPrint("\n[Test 4] IPv6 with interface name scope_id (%%ens33)\n");
  err = net_address_parse("[fe80::1%ens33]", NET_AF_INET6, &addr);
  DbgPrint("  Parse result: %d (expected -8)\n", err);

  // Тест полного адреса: IPv6 + scope_id + порт

  /*
  
  Name      InterfaceDescription                       ifIndex
  ----      --------------------                       -------
  Ethernet0 Intel(R) 82574L Gigabit Network Connection       9                                                                                                                                                                                                                                                                                                            PS C:\WINDOWS\system32> 
  
  */
  DbgPrint("\n[Test 5] Full address with scope and port (parse only, port "
           "ignored)\n");
  err = net_address_parse("[fe80::a921:2d1f:61fb:8942%Ethernet0]:9008", NET_AF_INET6, &addr);
  DbgPrint("  Parse result: %d (expected -3)\n", err);
  DbgPrint("  scope_id: %u (expected 9)\n", addr.scope_id);

  // Негативные тесты
  DbgPrint("\n[Test 6] Negative tests\n");

  err = net_address_parse("[fe80::1%13]", NET_AF_INET4, &addr);
  DbgPrint("  IPv6 addr with IPv4 family: %d (expected -8)\n", err);

  err = net_address_parse("[fe80::1%13]", NET_AF_INET6, NULL);
  DbgPrint("  NULL addr: %d (expected -8)\n", err);

  err = net_address_parse(NULL, NET_AF_INET6, &addr);
  DbgPrint("  NULL string: %d (expected -8)\n", err);

  err = net_address_parse("invalid::addr%13", NET_AF_INET6, &addr);
  DbgPrint("  Invalid address: %d (expected -8)\n", err);

  // Тест максимального scope_id
  DbgPrint("\n[Test 7] Maximum scope_id (4294967295)\n");
  err = net_address_parse("[fe80::1%4294967295]", NET_AF_INET6, &addr);
  DbgPrint("  Parse result: %d (expected -3)\n", err);
  DbgPrint("  scope_id: %u (expected 4294967295)\n", addr.scope_id);

  DbgPrint("\n[Test 8] Invalid IPV6 format (without [...] and without port)\n");
  err = net_address_parse("fe80::a921:2d1f:61fb:8942%Ethernet0",
                          NET_AF_INET6, &addr);
  DbgPrint("  Parse result: %d (expected -3)\n", err);
  DbgPrint("  scope_id: %u (expected 9)\n", addr.scope_id);

  DbgPrint("\n[Test 9] Invalid IPV6 format (without [...] and without port)\n");
  err = net_address_parse("fe80::a921:2d1f:61fb:8942%13", NET_AF_INET6,
                          &addr);
  DbgPrint("  Parse result: %d (expected -3)\n", err);
  DbgPrint("  scope_id: %u (expected 13)\n", addr.scope_id);

  DbgPrint("\n[Test 10] Invalid IPV6 format (without [...] but with port: ...%...::9090)\n");
  err = net_address_parse("fe80::a921:2d1f:61fb:8942%Ethernet0::9090",
                          NET_AF_INET6, &addr);
  DbgPrint("  Parse result: %d (expected -8)\n", err);

  DbgPrint("\n[Test] === All tests completed ===\n");

  DbgPrint("[Test] === Testing net_address_to_string with scope_id ===\n");

  char buffer[128];

  // IPv4
  DbgPrint("\n[Test 1] IPv4 to string\n");
  memset(&addr, 0, sizeof(addr));
  addr.family = NET_AF_INET4;
  addr.addr.ipv4 = 0x0100007F; // 127.0.0.1

  err = net_address_to_string(&addr, buffer, sizeof(buffer), FALSE);
  DbgPrint("  Result: %d (expected -3)\n", err);
  DbgPrint("  String: \"%s\" (expected \"127.0.0.1\")\n", buffer);

  // IPv4 с портом
  DbgPrint("\n[Test 2] IPv4 with port\n");
  addr.port = 0xCD21; // 52513
  err = net_address_to_string(&addr, buffer, sizeof(buffer), TRUE);
  DbgPrint("  Result: %d (expected -3)\n", err);
  DbgPrint("  String: \"%s\" (expected \"127.0.0.1:52513\")\n", buffer);

  // IPv6 без scope_id
  DbgPrint("\n[Test 3] IPv6 without scope_id\n");
  memset(&addr, 0, sizeof(addr));
  addr.family = NET_AF_INET6;
  // fe80::1
  uint8_t ipv6[] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  memcpy(addr.addr.ipv6, ipv6, 16);
  addr.scope_id = 0;

  err = net_address_to_string(&addr, buffer, sizeof(buffer), FALSE);
  DbgPrint("  Result: %d (expected -3)\n", err);
  DbgPrint("  String: \"%s\"\n", buffer);

  // IPv6 с числовым scope_id
  DbgPrint("\n[Test 4] IPv6 with numeric scope_id\n");
  addr.scope_id = 13;
  err = net_address_to_string(&addr, buffer, sizeof(buffer), FALSE);
  DbgPrint("  Result: %d (expected -3)\n", err);
  DbgPrint("  String: \"%s\" (expected [fe80::1%%13])\n", buffer);

  // IPv6 с scope_id и портом
  DbgPrint("\n[Test 5] IPv6 with scope_id and port\n");
  addr.port = 0xCD21; // 52513
  err = net_address_to_string(&addr, buffer, sizeof(buffer), TRUE);
  DbgPrint("  Result: %d (expected -3)\n", err);
  DbgPrint("  String: \"%s\" (expected [fe80::1%%13]:52513)\n", buffer);

  // Round-trip (parse -> to_string -> parse)
  DbgPrint("\n[Test 6] Round-trip test\n");

  const char *original = "[fe80::1%13]:9008";
  net_address_t addr1, addr2;

  DbgPrint("  Original: \"%s\"\n", original);

  err = net_address_parse(original, NET_AF_INET6, &addr1);
  if (err != NET_SUCCESS) {
    DbgPrint("  Parse failed: %d\n", err);
  } else {
    DbgPrint("  Parse OK: scope_id=%u\n", addr1.scope_id);

    err = net_address_to_string(&addr1, buffer, sizeof(buffer), TRUE);
    if (err != NET_SUCCESS) {
      DbgPrint("  To string failed: %d\n", err);
    } else {
      DbgPrint("  To string: \"%s\"\n", buffer);

      err = net_address_parse(buffer, NET_AF_INET6, &addr2);
      if (err == NET_SUCCESS && addr1.scope_id == addr2.scope_id) {
        DbgPrint("  Round-trip: SUCCESS\n");
      } else {
        DbgPrint("  Round-trip: FAILED\n");
      }
    }
  }

  // Негативные тесты
  DbgPrint("\n[Test 7] Negative tests\n");

  err = net_address_to_string(NULL, buffer, sizeof(buffer), FALSE);
  DbgPrint("  NULL addr: %d (expected -8)\n", err);

  err = net_address_to_string(&addr, NULL, sizeof(buffer), FALSE);
  DbgPrint("  NULL buffer: %d (expected -8)\n", err);

  char small_buf[4];
  err = net_address_to_string(&addr, small_buf, sizeof(small_buf), FALSE);
  DbgPrint("  Buffer too small: %d (expected -7)\n", err);

  DbgPrint("\n[Test] === All tests completed ===\n");


  return STATUS_SUCCESS;
}
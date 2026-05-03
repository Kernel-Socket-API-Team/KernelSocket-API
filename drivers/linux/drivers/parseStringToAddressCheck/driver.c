#include "../../../../include/ksockapi.h"

#define KDBG_PRINT(fmt, ...) printk(KERN_INFO fmt, ##__VA_ARGS__)

static void test_ipv4_to_string(void)
{
    net_address_t addr;
    net_error_t err;
    char buffer[128];
    
    KDBG_PRINT("\n[Test 1] IPv4 to string\n");
    memset(&addr, 0, sizeof(addr));
    addr.family = NET_AF_INET4;
    addr.addr.ipv4 = htonl(0x7F000001); // 127.0.0.1 в сетевом порядке
    
    err = net_address_to_string(&addr, buffer, sizeof(buffer), false);
    KDBG_PRINT("  Result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  String: \"%s\" (expected \"127.0.0.1\")\n", buffer);
    }
}

static void test_ipv4_with_port(void)
{
    net_address_t addr;
    net_error_t err;
    char buffer[128];
    
    KDBG_PRINT("\n[Test 2] IPv4 with port\n");
    memset(&addr, 0, sizeof(addr));
    addr.family = NET_AF_INET4;
    addr.addr.ipv4 = htonl(0x7F000001); // 127.0.0.1
    addr.port = htons(52513); // 52513 в сетевом порядке
    
    err = net_address_to_string(&addr, buffer, sizeof(buffer), true);
    KDBG_PRINT("  Result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  String: \"%s\" (expected \"127.0.0.1:52513\")\n", buffer);
    }
}

static void test_ipv6_without_scope(void)
{
    net_address_t addr;
    net_error_t err;
    char buffer[128];
    
    KDBG_PRINT("\n[Test 3] IPv6 without scope_id\n");
    memset(&addr, 0, sizeof(addr));
    addr.family = NET_AF_INET6;
    // fe80::1
    uint8_t ipv6[] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    memcpy(addr.addr.ipv6, ipv6, 16);
    addr.scope_id = 0;
    
    err = net_address_to_string(&addr, buffer, sizeof(buffer), false);
    KDBG_PRINT("  Result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  String: \"%s\"\n", buffer);
    }
}

static void test_ipv6_with_numeric_scope(void)
{
    net_address_t addr;
    net_error_t err;
    char buffer[128];
    
    KDBG_PRINT("\n[Test 4] IPv6 with numeric scope_id\n");
    memset(&addr, 0, sizeof(addr));
    addr.family = NET_AF_INET6;
    // fe80::1
    uint8_t ipv6[] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    memcpy(addr.addr.ipv6, ipv6, 16);
    addr.scope_id = 13;
    
    err = net_address_to_string(&addr, buffer, sizeof(buffer), false);
    KDBG_PRINT("  Result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  String: \"%s\" (expected \"fe80::1%%13\")\n", buffer);
    }
}

static void test_ipv6_with_scope_and_port(void)
{
    net_address_t addr;
    net_error_t err;
    char buffer[128];
    
    KDBG_PRINT("\n[Test 5] IPv6 with scope_id and port\n");
    memset(&addr, 0, sizeof(addr));
    addr.family = NET_AF_INET6;
    // fe80::1
    uint8_t ipv6[] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    memcpy(addr.addr.ipv6, ipv6, 16);
    addr.scope_id = 13;
    addr.port = htons(52513);
    
    err = net_address_to_string(&addr, buffer, sizeof(buffer), true);
    KDBG_PRINT("  Result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  String: \"%s\" (expected \"[fe80::1%%13]:52513\")\n", buffer);
    }
}

static void test_roundtrip(void)
{
    net_address_t addr1, addr2;
    net_error_t err;
    char buffer[128];
    
    KDBG_PRINT("\n[Test 6] Round-trip test\n");
    
    const char* original = "[fe80::1%13]:9008";
    KDBG_PRINT("  Original: \"%s\"\n", original);
    
    err = net_address_parse(original, NET_AF_INET6, &addr1);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  Parse failed: %d\n", err);
        return;
    }
    
    KDBG_PRINT("  Parse OK: scope_id=%u, port=%d\n", 
               addr1.scope_id, ntohs(addr1.port));
    
    err = net_address_to_string(&addr1, buffer, sizeof(buffer), true);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  To string failed: %d\n", err);
        return;
    }
    
    KDBG_PRINT("  To string: \"%s\"\n", buffer);
    
    err = net_address_parse(buffer, NET_AF_INET6, &addr2);
    if (err == NET_SUCCESS && 
        addr1.scope_id == addr2.scope_id &&
        addr1.port == addr2.port &&
        memcmp(addr1.addr.ipv6, addr2.addr.ipv6, 16) == 0) {
        KDBG_PRINT("  Round-trip: SUCCESS\n");
    } else {
        KDBG_PRINT("  Round-trip: FAILED (err=%d, scope1=%u, scope2=%u)\n", 
                   err, addr1.scope_id, addr2.scope_id);
    }
}

static void test_negative_cases(void)
{
    net_address_t addr;
    net_error_t err;
    char buffer[128];
    char small_buf[4];
    
    KDBG_PRINT("\n[Test 7] Negative tests\n");
    
    memset(&addr, 0, sizeof(addr));
    addr.family = NET_AF_INET4;
    addr.addr.ipv4 = htonl(0x7F000001);
    
    err = net_address_to_string(NULL, buffer, sizeof(buffer), false);
    KDBG_PRINT("  NULL addr: %d (expected -8)\n", err);
    
    err = net_address_to_string(&addr, NULL, sizeof(buffer), false);
    KDBG_PRINT("  NULL buffer: %d (expected -8)\n", err);
    
    err = net_address_to_string(&addr, small_buf, sizeof(small_buf), false);
    KDBG_PRINT("  Buffer too small: %d (expected -7)\n", err);
}

static int __init test_module_init(void)
{
    KDBG_PRINT("=== Testing net_address_to_string ===\n");
    
    test_ipv4_to_string();
    test_ipv4_with_port();
    test_ipv6_without_scope();
    test_ipv6_with_numeric_scope();
    test_ipv6_with_scope_and_port();
    test_roundtrip();
    test_negative_cases();
    
    KDBG_PRINT("\n=== All tests completed ===\n");
    return 0;
}

static void __exit test_module_exit(void)
{
    KDBG_PRINT("Test module unloaded\n");
}

module_init(test_module_init);
module_exit(test_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Network Address To String Test Driver");
MODULE_AUTHOR("Developer");
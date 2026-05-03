#include "../../../../include/ksockapi.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/ctype.h>

#define KDBG_PRINT(fmt, ...) printk(KERN_INFO fmt, ##__VA_ARGS__)

static void test_ipv4_with_scope(void)
{
    net_address_t addr;
    net_error_t err;
    
    KDBG_PRINT("\n[Test 1] IPv4 with scope (should ignore %%)\n");
    memset(&addr, 0, sizeof(addr));
    err = net_address_parse("192.168.1.100%ens33:4444", NET_AF_INET4, &addr);
    KDBG_PRINT("  Parse result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  IPv4 address: 0x%08X\n", ntohl(addr.addr.ipv4));
        KDBG_PRINT("  port: %d\n", ntohs(addr.port));
        KDBG_PRINT("  scope_id: %u (expected 0)\n", addr.scope_id);
    }
}

static void test_ipv6_without_scope(void)
{
    net_address_t addr;
    net_error_t err;
    
    KDBG_PRINT("\n[Test 2] IPv6 without scope_id\n");
    memset(&addr, 0, sizeof(addr));
    err = net_address_parse("[fe80::1]", NET_AF_INET6, &addr);
    KDBG_PRINT("  Parse result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  scope_id: %u (expected 0)\n", addr.scope_id);
        
        uint8_t expected_ipv6[] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
        int match = memcmp(addr.addr.ipv6, expected_ipv6, 16) == 0;
        KDBG_PRINT("  Address correct: %s\n", match ? "YES" : "NO");
    }
}

static void test_ipv6_numeric_scope(void)
{
    net_address_t addr;
    net_error_t err;
    
    KDBG_PRINT("\n[Test 3] IPv6 with numeric scope_id (%%13)\n");
    memset(&addr, 0, sizeof(addr));
    err = net_address_parse("[fe80::1%13]", NET_AF_INET6, &addr);
    KDBG_PRINT("  Parse result: %d (expected -3)\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  scope_id: %u (expected 13)\n", addr.scope_id);
        KDBG_PRINT("  Success: %s\n", (addr.scope_id == 13) ? "YES" : "NO");
    }
}

static void test_ipv6_full_address(void)
{
    net_address_t addr;
    net_error_t err;
    
    KDBG_PRINT("\n[Test 5] Full address with scope and port\n");
    memset(&addr, 0, sizeof(addr));
    err = net_address_parse("[fe80::a921:2d1f:61fb:8942%ens33]:9008", NET_AF_INET6, &addr);
    KDBG_PRINT("  Parse result: %d\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  scope_id: %u\n", addr.scope_id);
        KDBG_PRINT("  port: %d\n", ntohs(addr.port));
    }
}

static void test_negative_cases(void)
{
    net_address_t addr;
    net_error_t err;
    
    KDBG_PRINT("\n[Test 6] Negative tests\n");
    
    err = net_address_parse("[fe80::1%13]", NET_AF_INET4, &addr);
    KDBG_PRINT("  IPv6 addr with IPv4 family: %d (expected -8)\n", err);
    
    err = net_address_parse("[fe80::1%13]", NET_AF_INET6, NULL);
    KDBG_PRINT("  NULL addr: %d (expected -8)\n", err);
    
    err = net_address_parse(NULL, NET_AF_INET6, &addr);
    KDBG_PRINT("  NULL string: %d (expected -8)\n", err);
    
    err = net_address_parse("invalid::addr%13", NET_AF_INET6, &addr);
    KDBG_PRINT("  Invalid address: %d (expected -8)\n", err);
}

static void test_ipv6_without_brackets(void)
{
    net_address_t addr;
    net_error_t err;
    
    KDBG_PRINT("\n[Test 8] IPv6 without brackets (valid format)\n");
    memset(&addr, 0, sizeof(addr));
    err = net_address_parse("fe80::a921:2d1f:61fb:8942%ens33", NET_AF_INET6, &addr);
    KDBG_PRINT("  Parse result: %d\n", err);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  scope_id: %u\n", addr.scope_id);
    }
}

static int __init test_module_init(void)
{
    KDBG_PRINT("=== NET_ADDRESS_PARSE TEST SUITE ===\n");
    
    test_ipv4_with_scope();
    test_ipv6_without_scope();
    test_ipv6_numeric_scope();
    test_ipv6_full_address();
    test_negative_cases();
    test_ipv6_without_brackets();
    
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
MODULE_DESCRIPTION("Network Address Parse Test Driver");
MODULE_AUTHOR("Developer");
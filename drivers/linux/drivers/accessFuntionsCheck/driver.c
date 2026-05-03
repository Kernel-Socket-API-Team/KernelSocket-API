#include "../../../../include/ksockapi.h"
#include <linux/printk.h>
#include <linux/delay.h>

#define KDBG_PRINT(fmt, ...) printk(KERN_INFO fmt, ##__VA_ARGS__)
#define TEST_ASSERT(condition, msg, expected, got) \
    do { \
        if (condition) { \
            KDBG_PRINT("  OK %s\n", msg); \
        } else { \
            KDBG_PRINT("  NOT OK %s (expected %d, got %d)\n", msg, expected, got); \
        } \
    } while(0)

static net_address_t create_test_address_ipv4(const char* ip_str, uint16_t port)
{
    net_address_t addr;
    memset(&addr, 0, sizeof(addr));
    net_address_parse(ip_str, NET_AF_INET4, &addr);
    
    uint16_t net_port;
    net_htons(port, &net_port);
    addr.port = net_port;
    
    return addr;
}

static net_address_t create_test_address_ipv6(const char* ip_str, uint16_t port, uint32_t scope_id)
{
    net_address_t addr;
    memset(&addr, 0, sizeof(addr));
    net_address_parse(ip_str, NET_AF_INET6, &addr);
    
    uint16_t net_port;
    net_htons(port, &net_port);
    addr.port = net_port;
    addr.scope_id = scope_id;
    
    return addr;
}

static void test_get_address(void)
{
    net_socket_t* sock = NULL;
    net_address_t addr;
    net_address_t retrieved_addr;
    net_error_t err;
    
    KDBG_PRINT("\n=== Test 1: net_socket_get_address ===\n");
    
    // Создаем тестовый адрес
    addr = create_test_address_ipv4("192.168.0.255", 9001);
    
    // Создаем сокет
    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  Failed to create socket: %d\n", err);
        return;
    }
    KDBG_PRINT("  Socket created successfully\n");
    
    // Привязываем сокет
    err = net_socket_bind(sock, &addr);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  Failed to bind socket: %d\n", err);
        net_socket_close(sock);
        return;
    }
    KDBG_PRINT("  Socket bound to address\n");
    
    // Получаем адрес сокета
    err = net_socket_get_address(sock, &retrieved_addr);
    TEST_ASSERT(err == NET_SUCCESS, "net_socket_get_address success", NET_SUCCESS, err);
    
    if (err == NET_SUCCESS) {
        char buffer[NET_ADDRSTRLEN];
        net_address_to_string(&retrieved_addr, buffer, sizeof(buffer), true);
        KDBG_PRINT("  Retrieved address: %s\n", buffer);
        
        // Проверяем, что адрес совпадает
        int family_match = (retrieved_addr.family == addr.family);
        int port_match = (retrieved_addr.port == addr.port);
        int ip_match = (retrieved_addr.addr.ipv4 == addr.addr.ipv4);
        
        TEST_ASSERT(family_match, "Family matches", 1, family_match);
        TEST_ASSERT(port_match, "Port matches", 1, port_match);
        TEST_ASSERT(ip_match, "IP matches", 1, ip_match);
    }
    
    // Негативный тест: NULL параметры
    err = net_socket_get_address(NULL, &retrieved_addr);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL sock returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    err = net_socket_get_address(sock, NULL);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL addr returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    net_socket_close(sock);
}

static void test_get_remote_address(void)
{
    net_socket_t* sock = NULL;
    net_address_t local_addr;
    net_address_t remote_addr;
    net_address_t retrieved_addr;
    net_error_t err;
    
    KDBG_PRINT("\n=== Test 2: net_socket_get_remote_address ===\n");
    
    // Создаем тестовые адреса
    local_addr = create_test_address_ipv4("0.0.0.0", 8888);
    remote_addr = create_test_address_ipv4("8.8.8.8", 80);
    
    // Создаем сокет
    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  Failed to create socket: %d\n", err);
        return;
    }
    KDBG_PRINT("  Socket created successfully\n");
    
    // Привязываем локальный адрес
    err = net_socket_bind(sock, &local_addr);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  Failed to bind socket: %d\n", err);
        net_socket_close(sock);
        return;
    }
    
    // Пытаемся получить удаленный адрес до connect (должен быть недоступен)
    err = net_socket_get_remote_address(sock, &retrieved_addr);
    KDBG_PRINT("  Get remote address before connect: %d (expected error)\n", err);
    
    // Пытаемся соединиться (может не соединиться, но remote_addr должен сохраниться?)
    err = net_socket_connect(sock, &remote_addr);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  Connect failed (expected if no route): %d\n", err);
        // В любом случае, проверяем get_remote_address
    }
    
    // Получаем удаленный адрес
    err = net_socket_get_remote_address(sock, &retrieved_addr);
    TEST_ASSERT(err == NET_SUCCESS || err == NET_ERROR_INVALID_STATE, 
                "net_socket_get_remote_address call", NET_SUCCESS, err);
    
    if (err == NET_SUCCESS) {
        char buffer[NET_ADDRSTRLEN];
        net_address_to_string(&retrieved_addr, buffer, sizeof(buffer), true);
        KDBG_PRINT("  Retrieved remote address: %s\n", buffer);
    }
    
    // Негативный тест: NULL параметры
    err = net_socket_get_remote_address(NULL, &retrieved_addr);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL sock returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    err = net_socket_get_remote_address(sock, NULL);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL addr returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    net_socket_close(sock);
}

static void test_get_type_and_protocol(void)
{
    net_socket_t* tcp_sock = NULL;
    net_socket_t* udp_sock = NULL;
    net_socket_t* tcp_listen_sock = NULL;
    net_socket_type_t type;
    net_protocol_t protocol;
    net_error_t err;
    
    KDBG_PRINT("\n=== Test 3: net_socket_get_type and net_socket_get_protocol ===\n");
    
    // Тест TCP клиентского сокета
    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &tcp_sock);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  TCP client socket created\n");
        
        err = net_socket_get_type(tcp_sock, &type);
        TEST_ASSERT(err == NET_SUCCESS, "Get TCP client type success", NET_SUCCESS, err);
        TEST_ASSERT(type == NET_SOCK_TYPE_TCP_CONNECTION, "TCP client type correct", 
                    NET_SOCK_TYPE_TCP_CONNECTION, type);
        
        err = net_socket_get_protocol(tcp_sock, &protocol);
        TEST_ASSERT(err == NET_SUCCESS, "Get TCP client protocol success", NET_SUCCESS, err);
        TEST_ASSERT(protocol == NET_PROTO_TCP, "TCP client protocol correct", 
                    NET_PROTO_TCP, protocol);
        
        net_socket_close(tcp_sock);
    }
    
    // Тест UDP сокета
    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &udp_sock);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  UDP socket created\n");
        
        err = net_socket_get_type(udp_sock, &type);
        TEST_ASSERT(err == NET_SUCCESS, "Get UDP type success", NET_SUCCESS, err);
        TEST_ASSERT(type == NET_SOCK_TYPE_UDP, "UDP type correct", 
                    NET_SOCK_TYPE_UDP, type);
        
        err = net_socket_get_protocol(udp_sock, &protocol);
        TEST_ASSERT(err == NET_SUCCESS, "Get UDP protocol success", NET_SUCCESS, err);
        TEST_ASSERT(protocol == NET_PROTO_UDP, "UDP protocol correct", 
                    NET_PROTO_UDP, protocol);
        
        net_socket_close(udp_sock);
    }
    
    // Тест TCP слушающего сокета
    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &tcp_listen_sock);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  TCP listen socket created\n");
        
        err = net_socket_get_type(tcp_listen_sock, &type);
        TEST_ASSERT(err == NET_SUCCESS, "Get TCP listen type success", NET_SUCCESS, err);
        TEST_ASSERT(type == NET_SOCK_TYPE_TCP_LISTEN, "TCP listen type correct", 
                    NET_SOCK_TYPE_TCP_LISTEN, type);
        
        net_socket_close(tcp_listen_sock);
    }
    
    // Негативные тесты
    err = net_socket_get_type(NULL, &type);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL sock returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    err = net_socket_get_protocol(NULL, &protocol);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL sock returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
}

static void test_last_error_and_platform_error(void)
{
    net_socket_t* sock = NULL;
    net_error_t last_err;
    const void* platform_err;
    net_error_t err;
    
    KDBG_PRINT("\n=== Test 4: net_socket_last_error and net_socket_last_platform_error ===\n");
    
    // Создаем сокет
    err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);
    if (err != NET_SUCCESS) {
        KDBG_PRINT("  Failed to create socket: %d\n", err);
        return;
    }
    KDBG_PRINT("  Socket created successfully\n");
    
    // Проверяем начальное состояние ошибок
    err = net_socket_last_error(sock, &last_err);
    TEST_ASSERT(err == NET_SUCCESS, "Get last error success", NET_SUCCESS, err);
    TEST_ASSERT(last_err == NET_SUCCESS, "Initial error is NET_SUCCESS", NET_SUCCESS, last_err);
    
    err = net_socket_last_platform_error(sock, &platform_err);
    TEST_ASSERT(err == NET_SUCCESS, "Get platform error success", NET_SUCCESS, err);
    TEST_ASSERT(platform_err == NULL, "Initial platform error is NULL", 0, (long)platform_err);
    
    // Пытаемся выполнить операцию, которая должна вызвать ошибку
    net_address_t invalid_addr;
    memset(&invalid_addr, 0, sizeof(invalid_addr));
    invalid_addr.family = NET_AF_INET4;
    invalid_addr.addr.ipv4 = 0; // Невалидный адрес
    
    // connect с невалидным адресом (должна быть ошибка)
    err = net_socket_connect(sock, &invalid_addr);
    KDBG_PRINT("  Connect with invalid address returned: %d\n", err);
    
    // Проверяем, что ошибка сохранилась
    err = net_socket_last_error(sock, &last_err);
    TEST_ASSERT(err == NET_SUCCESS, "Get last error after failed operation", NET_SUCCESS, err);
    KDBG_PRINT("  Last error after failed operation: %d\n", last_err);
    
    err = net_socket_last_platform_error(sock, &platform_err);
    TEST_ASSERT(err == NET_SUCCESS, "Get platform error after failed operation", NET_SUCCESS, err);
    KDBG_PRINT("  Platform error pointer: %p\n", platform_err);
    
    // Выполняем успешную операцию (bind с валидным адресом)
    net_address_t valid_addr;
    memset(&valid_addr, 0, sizeof(valid_addr));
    valid_addr = create_test_address_ipv4("127.0.0.1", 9999);
    
    err = net_socket_bind(sock, &valid_addr);
    if (err == NET_SUCCESS) {
        KDBG_PRINT("  Successful bind operation\n");
        
        // Проверяем, что ошибки сбросились
        err = net_socket_last_error(sock, &last_err);
        TEST_ASSERT(err == NET_SUCCESS, "Get last error after success", NET_SUCCESS, err);
        TEST_ASSERT(last_err == NET_SUCCESS, "Error reset to NET_SUCCESS after success", 
                    NET_SUCCESS, last_err);
        
        err = net_socket_last_platform_error(sock, &platform_err);
        TEST_ASSERT(err == NET_SUCCESS, "Get platform error after success", NET_SUCCESS, err);
        TEST_ASSERT(platform_err == NULL, "Platform error reset to NULL after success", 
                    0, (long)platform_err);
    }
    
    // Негативные тесты
    err = net_socket_last_error(NULL, &last_err);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL sock returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    err = net_socket_last_error(sock, NULL);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL error returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    err = net_socket_last_platform_error(NULL, &platform_err);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL sock returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    err = net_socket_last_platform_error(sock, NULL);
    TEST_ASSERT(err == NET_ERROR_INVALID_PARAM, "NULL platform_error returns INVALID_PARAM", 
                NET_ERROR_INVALID_PARAM, err);
    
    net_socket_close(sock);
}

static void test_all_getters_on_invalid_socket(void)
{
    net_socket_t* sock = NULL;
    net_address_t addr;
    net_socket_type_t type;
    net_protocol_t protocol;
    net_error_t last_err;
    const void* platform_err;
    
    KDBG_PRINT("\n=== Test 5: All getters on invalid socket ===\n");
    
    // Создаем и закрываем сокет
    net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);
    if (sock) {
        net_socket_close(sock);
        KDBG_PRINT("  Socket created and closed\n");
    }
    
    // Теперь пытаемся использовать закрытый сокет
    net_error_t err = net_socket_get_address(sock, &addr);
    KDBG_PRINT("  net_socket_get_address on closed socket: %d\n", err);
    
    err = net_socket_get_remote_address(sock, &addr);
    KDBG_PRINT("  net_socket_get_remote_address on closed socket: %d\n", err);
    
    err = net_socket_get_type(sock, &type);
    KDBG_PRINT("  net_socket_get_type on closed socket: %d\n", err);
    
    err = net_socket_get_protocol(sock, &protocol);
    KDBG_PRINT("  net_socket_get_protocol on closed socket: %d\n", err);
    
    err = net_socket_last_error(sock, &last_err);
    KDBG_PRINT("  net_socket_last_error on closed socket: %d\n", err);
    
    err = net_socket_last_platform_error(sock, &platform_err);
    KDBG_PRINT("  net_socket_last_platform_error on closed socket: %d\n", err);
}

static int __init test_module_init(void)
{
    net_error_t err;
    
    KDBG_PRINT("\n========================================\n");
    KDBG_PRINT("=== NETWORK SOCKET GETTERS TEST SUITE ===\n");
    KDBG_PRINT("========================================\n");
    
    // Регистрируем библиотеку
    err = net_register();
    KDBG_PRINT("\nnet_register: %d\n", err);
    
    // Активируем библиотеку
    err = net_activate(1000);
    KDBG_PRINT("net_activate: %d\n", err);
    
    // Проверяем готовность
    err = net_is_ready();
    KDBG_PRINT("net_is_ready: %d\n", err);
    
    if (err != NET_SUCCESS) {
        KDBG_PRINT("Library not ready, aborting tests\n");
        net_cleanup();
        return -1;
    }
    
    // Запускаем тесты
    test_get_address();
    test_get_remote_address();
    test_get_type_and_protocol();
    test_last_error_and_platform_error();
    test_all_getters_on_invalid_socket();
    
    KDBG_PRINT("\n========================================\n");
    KDBG_PRINT("=== ALL TESTS COMPLETED ===\n");
    KDBG_PRINT("========================================\n");
    
    // Очищаем библиотеку
    net_cleanup();
    
    return 0;
}

static void __exit test_module_exit(void)
{
    KDBG_PRINT("Test module unloaded\n");
}

module_init(test_module_init);
module_exit(test_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Network Socket Getters Test Driver");
MODULE_AUTHOR("Developer");
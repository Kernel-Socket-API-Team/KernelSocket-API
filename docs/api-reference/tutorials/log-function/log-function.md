# Туториал 3: Функция диагностики сокета

## Введение

Данный туториал демонстрирует создание вспомогательной диагностической функции `socket_diagnostic()`, которая собирает и выводит полную информацию о состоянии сокета. Такая функция незаменима при отладке сетевого кода в среде ядра, где стандартные отладчики могут быть недоступны или неудобны в использовании.

## Ключевая концепция
Диагностическая функция показывает, как комбинировать различные access-функции библиотеки для получения целостной картины состояния сокета:

* **Геттеры типа и протокола** определяют природу сокета (TCP/UDP, слушающий/соединённый)

* **Геттеры адресов** показывают, к каким конечным точкам привязан сокет

* **Геттеры ошибок** предоставляют как высокоуровневый код ошибки API, так и низкоуровневую платформозависимую информацию

Особенность данной функции — её кроссплатформенность: она корректно работает и в Windows (с `DbgPrint` и `NTSTATUS`), и в Linux (с `printk` и `errno`), автоматически адаптируя вывод под целевую платформу.

## Код функции
```c
#ifdef _WIN32
#include <ntstrsafe.h>
#define diag_snprintf RtlStringCbPrintfA
#else
#include <linux/kernel.h>
#define diag_snprintf snprintf
#endif

#ifdef _WIN32
#define diag_print DbgPrint
#else
#define diag_print printk
#endif

void socket_diagnostic(net_socket_t* sock, const char* tag)
{
    net_error_t err;
    net_socket_type_t type;
    net_protocol_t protocol;
    net_address_t local_addr;
    net_address_t remote_addr;
    net_error_t last_error;
    const void* platform_error;
    
    char local_ip[NET_ADDRSTRLEN];
    char remote_ip[NET_ADDRSTRLEN];
    
    if (!sock) {
        diag_print("[DIAG] %s: socket is NULL!\n", tag);
        return;
    }
    
    diag_print("[DIAG] === %s (sock=%p) ===\n", tag, sock);
    
    // Тип сокета
    err = net_socket_get_type(sock, &type);
    if (err == NET_SUCCESS) {
        const char* type_str = "UNKNOWN";
        switch (type) {
            case NET_SOCK_TYPE_TCP_LISTEN:     type_str = "TCP_LISTEN"; break;
            case NET_SOCK_TYPE_TCP_CONNECTION: type_str = "TCP_CONNECTION"; break;
            case NET_SOCK_TYPE_UDP:            type_str = "UDP"; break;
        }
        diag_print("  type: %s (%d)\n", type_str, type);
    } else {
        diag_print("  net_socket_get_type FAILED: %d\n", err);
    }
    
    // Протокол
    err = net_socket_get_protocol(sock, &protocol);
    if (err == NET_SUCCESS) {
        const char* proto_str = (protocol == NET_PROTO_TCP) ? "TCP" : "UDP";
        diag_print("  protocol: %s (%d)\n", proto_str, protocol);
    } else {
        diag_print("  net_socket_get_protocol FAILED: %d\n", err);
    }
    
    // Локальный адрес
    err = net_socket_get_address(sock, &local_addr);
    if (err == NET_SUCCESS) {
        net_address_to_string(&local_addr, local_ip, sizeof(local_ip), 1);
        diag_print("  local address: %s\n", local_ip);
    } else {
        diag_print("  net_socket_get_address FAILED: %d\n", err);
    }
    
    // Удалённый адрес
    err = net_socket_get_remote_address(sock, &remote_addr);
    if (err == NET_SUCCESS) {
        net_address_to_string(&remote_addr, remote_ip, sizeof(remote_ip), 1);
        diag_print("  remote address: %s\n", remote_ip);
    } else {
        diag_print("  net_socket_get_remote_address FAILED: %d\n", err);
    }
    
    // Последняя ошибка
    err = net_socket_last_error(sock, &last_error);
    if (err == NET_SUCCESS) {
        diag_print("  last error: %d\n", last_error);
    } else {
        diag_print("  net_socket_last_error FAILED: %d\n", err);
    }
    
    // Платформозависимая ошибка
    err = net_socket_last_platform_error(sock, &platform_error);
    if (err == NET_SUCCESS) {
#ifdef _WIN32
        diag_print("  platform error (NTSTATUS): 0x%08X\n", *(NTSTATUS*)platform_error);
#else
        diag_print("  platform error (errno): %d\n", *(int*)platform_error);
#endif
    } else {
        diag_print("  net_socket_last_platform_error FAILED: %d\n", err);
    }
    
    diag_print("[DIAG] === end ===\n");
}
```

## Пояснения

| Функция | Назначение |
|---------|------------|
| `net_socket_get_type` | Получение типа сокета (слушающий TCP, соединённый TCP или UDP) |
| `net_socket_get_protocol` | Получение протокола сокета (TCP или UDP) |
| `net_socket_get_address` | Получение локального адреса и порта сокета |
| `net_socket_get_remote_address` | Получение удалённого адреса и порта (для соединённых сокетов) |
| `net_socket_last_error` | Получение последней ошибки API (высокоуровневый код) |
| `net_socket_last_platform_error` | Получение платформозависимой ошибки (`NTSTATUS` для Windows, `errno` для Linux) |
| `net_address_to_string` | Преобразование бинарного адреса в человекочитаемую строку |

## Платформенные особенности

Особенность данного туториала — явная демонстрация работы с платформозависимыми макросами:

| Компонент | Windows | Linux |
|-----------|---------|-------|
| Вывод диагностики | `DbgPrint` | `printk` |
| Форматирование строк | `RtlStringCbPrintfA` (через `diag_snprintf`) | `snprintf` |
| Тип платформенной ошибки | `NTSTATUS` (выводится в hex) | `errno` (выводится как int) |

Функция не требует отдельной точки входа и предназначена для вызова из любого места драйвера/модуля ядра при подозрении на проблемы с сетевым взаимодействием.
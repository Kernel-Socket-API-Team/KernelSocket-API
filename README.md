# KernelSocket API

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue)]()

Единый C API для работы с TCP/UDP сокетами на уровне ядра Windows и Linux. Позволяет использовать один и тот же код для обеих платформ.

## Назначение

Разработка драйверов и модулей ядра, требующих сетевого взаимодействия, без необходимости дублировать реализацию под каждую ОС. Библиотека абстрагирует различия между WSK (Windows) и kernel sockets (Linux).

## Возможности

- Поддержка TCP и UDP протоколов.
- Поддержка IPv4 и IPv6.
- Единый интерфейс для создания сокетов, привязки, установления соединения.
- Единые функции отправки и приёма данных.
- Вспомогательные функции для работы с адресами (парсинг, форматирование).
- Получение диагностической информации о сокете (тип, протокол, адреса, коды ошибок).
- Кроссплатформенная обработка ошибок с доступом к нативной ошибке ОС.
- Блокирующие сетевые операции с поддержкой бесконечного ожидания.

---

## Быстрый пример

UDP эхо сервер:

```c
#include <ksockapi.h>

static int udp_echo_server_thread(void* data) {
    net_socket_t* sock;
    net_address_t local_addr, client_addr;
    char buffer[256];
    size_t received_len;
    
    net_activate(NET_WAIT_INFINITE);
    
    net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &sock);
    net_address_parse("0.0.0.0:8888", NET_AF_INET4, &local_addr);
    net_socket_bind(sock, &local_addr);
    
    while (1) {
        net_socket_receive(sock, buffer, sizeof(buffer), &client_addr, &received_len);
        net_socket_connect(sock, &client_addr);
        net_socket_send(sock, buffer, received_len, NULL);
    }
    
    net_cleanup();
    return 0;
}

```
Полные примеры: [UDP эхо сервер](./docs/api-reference/tutorials/udp-echo-server/udp-echo-server.md), [TCP сервер с IPv6](./docs/api-reference/tutorials/tcp-server/tcp-server.md), [функция диагностики сокета](./docs/api-reference/tutorials/log-function/log-function.md).

## Основные функции API

**Инициализация и завершение**

* `net_register()` — регистрация библиотеки в системе (вызов при загрузке драйвера).

* `net_activate()` — активация библиотеки в рабочем потоке.

* `net_is_ready()` — проверка готовности библиотеки.

* `net_cleanup()` — завершение работы и освобождение ресурсов.

**Создание и управление сокетами**

* `net_socket_create()` — создание сокета.

* `net_socket_close()` — закрытие сокета.

**Привязка и соединение**

* `net_socket_bind()` — привязка сокета к адресу.

* `net_socket_connect()` — установление TCP соединения (для UDP — сохранение адреса получателя).

**Передача данных**

* `net_socket_send()` — отправка данных.

* `net_socket_receive()` — приём данных.

* `net_socket_accept()` — принятие входящего TCP соединения.

**Работа с адресами**

* `net_address_parse()` — преобразование строки в структуру адреса.

* `net_address_to_string()` — преобразование структуры адреса в строку.

* `net_htons()` / `net_ntohs()` — преобразование порта между порядками байт.

**Диагностика**

* `net_socket_get_address()` — получение локального адреса сокета.

* `net_socket_get_remote_address()` — получение удалённого адреса.

* `net_socket_get_type()` — получение типа сокета.

* `net_socket_get_protocol()` — получение протокола.

* `net_socket_last_error()` — получение последней ошибки API.

* `net_socket_last_platform_error()` — получение платформозависимой ошибки.

## Сборка и подключение

### Linux
**Требования:** ядро Linux 5.4+, linux-headers, make.

**Подключение к проекту:**

```makefile
ccflags-y += -I KernelSocket-API/include
ccflags-y += -I KernelSocket-API/src
my_driver-y += KernelSocket-API/lib/knet.o
```

Файл `knet.o` объединяет все исходники библиотеки и компилируется вместе с модулем.

### Windows
**Требования:** Visual Studio 2022, WDK, MSVC C++ libraries (Spectre-mitigated).

**Подключение к проекту:**

1. Скомпилировать `knet.lib` из решения в папке `lib`.

2. Добавить путь к `include/` в дополнительные каталоги включаемых файлов.

3. Добавить `knet.lib` в дополнительные зависимости компоновщика.

4. Добавить `netio.lib` в дополнительные зависимости.

## Пример точки входа

### Windows (драйвер)
```c
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    DriverObject->DriverUnload = DriverUnload;
    net_register();
    PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                         (PKSTART_ROUTINE)tcp_server_thread, &g_ctx);
    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    g_ctx.should_stop = 1;
    net_cleanup();
}
```

### Linux (драйвер)
```c
static int __init module_init(void) {
    net_register();
    g_WorkerThread = kthread_run(tcp_server_thread, &g_ctx, "tcp_server");
    return 0;
}

static void __exit module_exit(void) {
    g_ctx.should_stop = 1;
    kthread_stop(g_WorkerThread);
    net_cleanup();
}

module_init(module_init);
module_exit(module_exit);
MODULE_LICENSE("GPL");
```

## Коды ошибок 

| Код | Значение |
|-----|----------|
| `NET_SUCCESS` | Успешное выполнение операции |
| `NET_ERROR_NOT_INITIALIZED` | Библиотека не инициализирована |
| `NET_ERROR_NOT_REGISTER` | Библиотека не зарегистрирована |
| `NET_ERROR_NO_MEMORY` | Недостаточно памяти |
| `NET_ERROR_ACCESS_DENIED` | Доступ запрещён |
| `NET_ERROR_TIMEOUT` | Превышено время ожидания |
| `NET_ERROR_BUFFER_TOO_SMALL` | Размер буфера слишком мал |
| `NET_ERROR_INVALID_PARAM` | Некорректный параметр |
| `NET_ERROR_INVALID_STATE` | Сокет в невалидном состоянии |
| `NET_ERROR_INVALID_PROTOCOL` | Некорректный протокол |

## Лицензия

[MIT License](./LICENSE). Свободное использование, модификация и распространение.

## Документация
Полная документация, включая туториалы и справочник API, находится в папке [`docs/`](./docs/README.md).
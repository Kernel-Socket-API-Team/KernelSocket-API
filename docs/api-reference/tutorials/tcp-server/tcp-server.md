# Туториал 2: TCP-сервер с поддержкой IPv6

## Введение

Данный туториал демонстрирует создание TCP-сервера, который слушает входящие подключения на порту 8888, принимает данные от клиента и отправляет их обратно (эхо). Особое внимание уделяется поддержке IPv6, что позволяет серверу работать в современных сетях с протоколом следующего поколения.

## Ключевая концепция

В отличие от UDP, где один сокет может одновременно принимать и отправлять данные, TCP требует разделения ролей:

* Слушающий сокет (`NET_SOCK_TYPE_TCP_LISTEN`) — ожидает входящие подключения

* Клиентский сокет (`NET_SOCK_TYPE_TCP_CONNECTION`) — создаётся после accept для общения с конкретным клиентом

## Код сервера

```c
#ifdef _WIN32
#include <ntstrsafe.h>
#define net_snprintf RtlStringCbPrintfA
#else
#define net_snprintf snprintf
#endif

static int tcp_server_thread(void* data)
{
    thread_context_t* ctx = (thread_context_t*)data;
    net_error_t err;
    net_address_t local_addr;
    net_address_t client_addr;
    char buffer[256];
    char response[256];
    size_t received_len;
    size_t sent_len;
    char client_ip[NET_ADDRSTRLEN];
    
    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
        return -1;

    // Создание слушающего сокета (IPv6)
    err = net_socket_create(NET_AF_INET6, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &ctx->listen_sock);
    if (err != NET_SUCCESS)
    {
        net_cleanup();
        return -1;
    }

    // Привязка к IPv6 адресу (порт 8888)
    err = net_address_parse("[::0]:8888", NET_AF_INET6, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->listen_sock);
        ctx->listen_sock = NULL;
        net_cleanup();
        return -1;
    }

    err = net_socket_bind(ctx->listen_sock, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->listen_sock);
        ctx->listen_sock = NULL;
        net_cleanup();
        return -1;
    }

    // Основной цикл приёма клиентов
    while (!ctx->should_stop)
    {
        err = net_socket_accept(ctx->listen_sock, &ctx->client_sock);
        if (err != NET_SUCCESS) continue;

        // Получаем адрес клиента
        err = net_socket_get_remote_address(ctx->client_sock, &client_addr);
        if (err != NET_SUCCESS)
        {
            net_socket_close(ctx->client_sock);
            ctx->client_sock = NULL;
            continue;
        }
        
        net_address_to_string(&client_addr, client_ip, sizeof(client_ip), 1);
        
        while (!ctx->should_stop)
        {
            err = net_socket_receive(ctx->client_sock, buffer, sizeof(buffer) - 1, NULL, &received_len);
            
            if (err != NET_SUCCESS || received_len == 0) break;

            if (received_len > 0)
            {
                buffer[received_len] = '\0';
                
                net_snprintf(response, sizeof(response), "[Client %s]: %s\n", client_ip, buffer);
                net_socket_send(ctx->client_sock, response, strlen(response), &sent_len);
            }
        }

        net_socket_close(ctx->client_sock);
        ctx->client_sock = NULL;
    }

    net_socket_close(ctx->listen_sock);
    ctx->listen_sock = NULL;

    return 0;
}
```

## Пояснение функций

| Функция | Назначение |
|---------|------------|
| `net_activate` | Активация библиотеки в рабочем потоке |
| `net_socket_create` | Создание слушающего TCP-сокета с типом `NET_SOCK_TYPE_TCP_LISTEN` |
| `net_address_parse` | Преобразование `[::0]:8888` в структуру адреса (IPv6) |
| `net_socket_bind` | Привязка сокета к порту 8888 на всех IPv6-интерфейсах |
| `net_socket_accept` | Блокирующий вызов; ожидает входящее подключение |
| `net_socket_get_remote_address` | Получение IPv6-адреса подключившегося клиента |
| `net_socket_receive` | Приём данных от клиента |
| `net_socket_send` | Отправка ответа (эхо) с указанием IP клиента |
| `net_socket_close` | Закрытие клиентского и слушающего сокетов |

## Платформенные особенности
Подробное описание различий в механизмах выгрузки между Windows и Linux приведено в разделе [Платформенное различие в поведении библиотеки](../../concepts.md).
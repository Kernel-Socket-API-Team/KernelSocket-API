# Туториал 1: UDP-эхо сервер

## Введение

Данный туториал демонстрирует создание простого UDP-сервера, который принимает дейтаграммы от клиентов и отправляет их обратно (эхо). На примере UDP-сокета показана его универсальность: один и тот же сокет используется как для приёма, так и для отправки данных, без необходимости создавать отдельные сокеты для разных направлений.

## Ключевая концепция
UDP-сокеты в данном API обладают важной особенностью: один сокет может одновременно слушать входящие дейтаграммы и отправлять ответы. Это достигается за счёт того, что:

* `net_socket_receive` возвращает адрес отправителя
* `net_socket_connect` сохраняет этот адрес для последующей отправки
* `net_socket_send` использует сохранённый адрес

Таким образом, для каждого клиента не требуется создавать новый сокет — достаточно переподключить существующий.

## Код сервера

```c
static int udp_echo_server_thread(void* data)
{
    thread_context_t* ctx = (thread_context_t*)data;
    net_error_t err;
    net_address_t local_addr;
    net_address_t client_addr;
    char buffer[256];
    size_t received_len;
    size_t sent_len;
    char addr_str[NET_ADDRSTRLEN];

    err = net_activate(NET_WAIT_INFINITE);
    if (err != NET_SUCCESS)
        return -1;

    err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &ctx->sock);
    if (err != NET_SUCCESS)
    {
        net_cleanup();
        return -1;
    }

    err = net_address_parse("0.0.0.0:8888", NET_AF_INET4, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->sock);
        ctx->sock = NULL;
        net_cleanup();
        return -1; 
    }

    err = net_socket_bind(ctx->sock, &local_addr);
    if (err != NET_SUCCESS)
    {
        net_socket_close(ctx->sock);
        ctx->sock = NULL;
        net_cleanup();
        return -1; 
    }

    while (!ctx->should_stop)
    {
        err = net_socket_receive(ctx->sock, buffer, sizeof(buffer), &client_addr, &received_len);

        if (err != NET_SUCCESS)
            continue;

        net_address_to_string(&client_addr, addr_str, sizeof(addr_str), 0);

        if (received_len > 0)
            buffer[received_len] = '\0';

        err = net_socket_connect(ctx->sock, &client_addr);
        if (err != NET_SUCCESS)
            continue;

        err = net_socket_send(ctx->sock, buffer, received_len, &sent_len);
    }

    net_cleanup();
    return 0;
}
```

## Пояснения

| Функция | Назначение |
|---------|------------|
| `net_activate` | Активация библиотеки в рабочем потоке |
| `net_socket_create` | Создание UDP-сокета типа `NET_SOCK_TYPE_UDP` |
| `net_address_parse` | Преобразование строки `0.0.0.0:8888` в структуру адреса |
| `net_socket_bind` | Привязка сокета к локальному адресу и порту |
| `net_socket_receive` | Приём дейтаграммы; заполняет `client_addr` адресом отправителя |
| `net_socket_connect` | Для UDP — сохраняет адрес получателя в сокете |
| `net_socket_send` | Отправка данных сохранённому получателю |

## Платформенные особенности
Подробное описание различий в механизмах выгрузки между Windows и Linux приведено в разделе [Платформенное различие в поведении библиотеки](../../concepts.md).
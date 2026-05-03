# Создание и настройка сервера

В данном разделе описываются все возможности создания серверных функций и работы с ними. API предоставляет возможность настройки TCP- и UDP-серверов с поддержкой IPv4 и IPv6. При этом создание сервера для протокола TCP осуществляется двумя функциями, а для UDP — одной.

## Обзор функций

```c
net_error_t net_socket_accept(net_socket_t* server, net_socket_t** client_out);

net_error_t net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received);
```

## Общий порядок действий

Инициализация сервера для TCP происходит по следующему принципу:
* Создаётся слушающий сокет (`NET_SOCK_TYPE_TCP_LISTEN`);
* Ожидается подключение клиента с помощью `net_socket_accept(...)`, при этом поток блокируется;
* После подключения клиента данные от него получаются через `net_socket_receive(...)` с использованием нового сокета, который вернул `net_socket_accept(...)`.

Инициализация сервера для UDP происходит по следующему принципу:
* Создаётся обычный UDP-сокет (`NET_SOCK_TYPE_UDP`);
* Входящие данные принимаются с использованием `net_socket_receive(...)`.

**Важно!** В случае принудительного закрытия серверного сокета (`net_socket_close(...)`) все операции ожидания клиента (`net_socket_accept(...)`) и ожидания сообщений (`net_socket_receive(...)`) прерываются. В случае работы через TCP, если соединение уже было установлено, сокет ожидает либо принудительного завершения соединения, либо любого сообщения от пользователя. После этого он принудительно отключается от сервера, а сокет удаляется.

## net_socket_accept
```c
net_error_t net_socket_accept(net_socket_t* server, net_socket_t** client_out);
```

### Описание
Принимает входящее TCP-соединение. **Блокирует выполнение потока** до установки нового соединения. Возвращает новый сокет для взаимодействия с клиентом. Адрес клиента (IPv4 или IPv6) автоматически сохраняется в сокете и может быть получен через `net_socket_get_address()`.

### Параметры

 * **[in]** `server` — слушающий сокет (должен быть создан с `NET_SOCK_TYPE_TCP_LISTEN`)
 * **[out]** `client_out` — новый сокет для общения с клиентом

### Возвращаемые значения
 * `NET_SUCCESS` — новое соединение принято
 * `NET_ERROR_INVALID_PARAM` — некорректные параметры (`server` или `client_out == NULL`)
 * `NET_ERROR_INVALID_PROTOCOL` — `server` не является TCP сокетом
 * `NET_ERROR_INVALID_STATE` — сокет не в режиме прослушивания или ошибка контекста
 * `NET_ERROR_NO_MEMORY` — недостаточно памяти для нового сокета
 * [Другие ошибки связанные с работой библиотек из раздела платформенных ошибок](./errors.md)

## net_socket_receive

```c
net_error_t net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received);
```

### Описание

Принимает данные через сокет. 
* Для TCP-сокета: читает данные от подключенного клиента.
* Для UDP-сокета: читает дейтаграмму и возвращает адрес отправителя.

**Важно!** Данная функция не выполняет автоматический `accept` для TCP-слушающих сокетов. Для TCP-сервера необходимо сначала вызвать `net_socket_accept()` для получения клиентского сокета, а затем использовать этот сокет для вызова `net_socket_receive()`.

### Параметры

 * **[in]** `sock` — сокет для приема (должен быть TCP connected или UDP)
 * **[out]** `buffer` — буфер для данных
 * **[in]** `buffer_size` — размер буфера
 * **[out]** `from_addr` — адрес отправителя (для UDP) или NULL. Для TCP сокета этот параметр можно игнорировать, так как адрес клиента можно получить через `net_socket_get_address()` после `accept`.
 * **[out]** `received` — количество реально принятых байт

### Возвращаемые значения

 * `NET_SUCCESS` — данные получены
 * `NET_ERROR_INVALID_PARAM` — некорректные параметры (`sock`, `buffer` или `buffer_size`)
 * `NET_ERROR_INVALID_STATE` — сокет не готов к приему данных
 * `NET_ERROR_NO_MEMORY` — недостаточно памяти для буфера
* [Другие ошибки связанные с работой библиотек из раздела платформенных ошибок](./errors.md)

## Примеры использования

### Создание UDP сервера использующего IPV4

```c
// Вызываем данную функцию в отдельном исполняющемся потоке!

#define PORT_UDP = 4445
#define BUFFER_SIZE 1024

net_error_t err;
net_address_t addr;
net_address_t client_addr;
char buffer[BUFFER_SIZE];
size_t received = 0;

// Инициализация библиотеки
err = net_activate(NET_WAIT_INFINITE);
if (err != NET_SUCCESS) goto exit;

// Создание UDP сокета
err = net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &g_ServerSockUDP);
if (err != NET_SUCCESS) goto exit;

addr.family = NET_AF_INET4;      // Устанавливается тип адресса (IPV4)
net_htons(PORT_UDP, &addr.port); // Привязка к порту  
addr.addr.ipv4 = 0;              // Принимаем сообщение от всех IPV4 адрессов
  
err = net_socket_bind(g_ServerSockUDP, &addr);
if (err != NET_SUCCESS) goto close_socket;

// Главный цикл сервера
// g_Running можно объявить глобально как volitile 
while (g_Running) {
    received = 0;
    err = net_socket_receive(g_ServerSockUDP, buffer, BUFFER_SIZE - 1, &client_addr, &received);

    if (err != NET_SUCCESS) 
        continue;

    if (received > 0)
        buffer[received] = '\0';
}

close_socket:
    if (g_ServerSockUDP) {
        net_socket_close(g_ServerSockUDP);
        g_ServerSockUDP = NULL;
    }

exit:
    // Закрытие потока ... 
```

### Модификация UDP сервера для использования IPV6
```c
// Для того чтобы настроить сервер для приема IPV6 проведем следующие модификации:

// Вместо NET_AF_INET4 используем NET_AF_INET6
err = net_socket_create(NET_AF_INET6, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &g_ServerSockUDP);

//...

// Модифицируем адресс сервера:
addr.family = NET_AF_INET6;      // NET_AF_INET6 вместо NET_AF_INET4
net_htons(PORT_UDP, &addr.port); // Привязка к порту (не изм.) 
memset(addr.addr.ipv6, 0, 16)    // Принимаем все IPV6 адреса
```

### Создание TCP сервера использующего IPV4

```c
#define PORT_TCP = 4444
#define BUFFER_SIZE 1024

// Вызываем данную функцию в отдельном исполняющемся потоке!

net_error_t err;
net_address_t addr;
net_socket_t *client_sock = NULL;
char buffer[BUFFER_SIZE];
size_t received;

// Инициализация библиотеки
err = net_activate(NET_WAIT_INFINITE);
if (err != NET_SUCCESS) goto exit;

// Создание сокета
err = net_socket_create(NET_AF_INET4, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &g_ServerSockTCP);
if (err != NET_SUCCESS) goto exit;

addr.family = NET_AF_INET4;       // Устанавливается тип адресса (IPV4)
net_htons(PORT_TCP, &addr.port);  // Привязка к порту
addr.addr.ipv4 = 0;               // Принимаем сообщение от всех IPV4 адрессов

err = net_socket_bind(g_ServerSockTCP, &addr);
if (err != NET_SUCCESS) goto close_server;

// Главный цикл сервера
// g_Running можно объявить глобально как volitile 
while (g_Running) {
    err = net_socket_accept(g_ServerSockTCP, &client_sock);
    if (err != NET_SUCCESS)
        continue;

    // Цикл приема данных от клиента
    while (g_Running) {
        received = 0;
        net_address_t client_addr;
        err = net_socket_receive(client_sock, buffer, BUFFER_SIZE - 1, &client_addr, &received);

        if (err != NET_SUCCESS)
            break;

        // Клиент отключился
        if (received == 0) 
            break;
      
        buffer[received] = '\0';
    }

    net_socket_close(client_sock);
    client_sock = NULL;
  }

close_server:
  if (g_ServerSockTCP) {
    net_socket_close(g_ServerSockTCP);
    g_ServerSockTCP = NULL;
  }

exit:
    // Закрытие потока ... 
```

### Ммодификация TCP сервера для использования IPV6

```c
// Для того чтобы настроить сервер для приема IPV6 проведем следующие модификации:

// Вместо NET_AF_INET4 используем NET_AF_INET6
err = net_socket_create(NET_AF_INET6, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_LISTEN, &g_ServerSockTCP);

//...

// Модифицируем адресс сервера:
addr.family = NET_AF_INET6;      // NET_AF_INET6 вместо NET_AF_INET4
net_htons(PORT_TCP, &addr.port); // Привязка к порту (не изм.) 
memset(addr.addr.ipv6, 0, 16)    // Принимаем все IPV6 адреса
```

### Использование net_address_parse для настройки сервера

Вместо ручного заполнения полей структуры `net_address_t` (установка `family`, обнуление `addr.ipv6`, указание порта) рекомендуется использовать функцию `net_address_parse`. Это делает код более читаемым, менее подверженным ошибкам и позволяет легко работать с IPv6 link-local адресами, где требуется указание `scope_id`.

```c
// Вместо ручного заполнения структуры следует использовать net_address_parse
net_address_parse("::", NET_AF_INET6, &addr);
// или
net_address_parse("0.0.0.0", NET_AF_INET4, &addr);

// И в случае использования scope_id

// Через числовой индекс интерфейса
net_address_parse("[fe80::1%9]", NET_AF_INET6, &addr);

// Через имя интерфейса (преобразуется в индекс автоматически)
net_address_parse("[fe80::1%Ethernet0]", NET_AF_INET6, &addr); 
```
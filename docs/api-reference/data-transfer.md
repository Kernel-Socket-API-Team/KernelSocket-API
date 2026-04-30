# Передача данных

Данный раздел описывает функции для установки соединения и отправки данных через сокет.

## Обзор функций

```c
net_error_t net_socket_connect(net_socket_t* sock, const net_address_t* addr);

net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);
```

---

## net_socket_connect

```c
net_error_t net_socket_connect(net_socket_t* sock, const net_address_t* addr);
```

### Описание

Устанавливает соединение с удалённым хостом и/или только сохраняет адрес назначения внутри сокета для TCP/UDP соответственно.
Вызов функции обязателен для каждого из протоколов.

Поведение зависит от протокола:

- **TCP** — выполняет реальное трёхстороннее рукопожатие (SYN - SYN-ACK - ACK). Функция блокирует выполнение до установки соединения или ошибки.
- **UDP** — реального соединения нет. Функция только сохраняет адрес получателя внутри сокета. Последующие вызовы `net_socket_send` будут использовать этот адрес автоматически.

**Важно!** Перед вызовом `net_socket_connect` можно явно привязать сокет через `net_socket_bind`. Без явного bind некоторые платформы могут не выбрать локальный адрес автоматически.

### Параметры

- **[in]** `sock` — сокет для подключения (должен быть создан с `NET_SOCK_TYPE_TCP_CONNECTION` или `NET_SOCK_TYPE_UDP`)
- **[in]** `addr` — адрес удалённого хоста. Для IPv6 link-local адресов (`fe80::`) поле `scope_id` должно быть заполнено — см. [Работа с адресами](./addressing.md)

### Возвращаемые значения

- `NET_SUCCESS` — соединение установлено (TCP) или адрес сохранён (UDP)
- `NET_ERROR_INVALID_PARAM` — некорректные параметры
- `NET_ERROR_INVALID_STATE` — сокет в неподходящем состоянии или контекст повреждён
- `NET_ERROR_INVALID_PROTOCOL` — вызов для слушающего TCP сокета (`NET_SOCK_TYPE_TCP_LISTEN`)
- `NET_ERROR_TIMEOUT` — превышено время ожидания соединения (только TCP)
- [Другие платформенные ошибки](./errors.md)

---

## net_socket_send

```c
net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);
```

### Описание

Отправляет данные через сокет.

Поведение зависит от протокола:

- **TCP** — отправляет данные в установленное предварительно соединение через `net_socket_connect`.
- **UDP** — отправляет датаграмму на адрес, сохранённый при вызове `net_socket_connect`. Адрес должен быть установлен заранее — без него функция вернёт `NET_ERROR_INVALID_STATE`.

### Параметры

- **[in]** `sock` — сокет для отправки
- **[in]** `data` — указатель на отправляемые данные
- **[in]** `size` — размер данных в байтах (должен быть больше 0)
- **[out]** `sent` — количество реально отправленных байт. Может быть `NULL` если результат не нужен

### Возвращаемые значения

- `NET_SUCCESS` — данные отправлены
- `NET_ERROR_INVALID_PARAM` — `sock` или `data` равны `NULL`, либо `size` равен 0
- `NET_ERROR_INVALID_STATE` — сокет не готов к отправке, контекст повреждён, или для UDP не установлен адрес получателя
- [Другие платформенные ошибки](./errors.md)

---

## Примеры использования

### UDP — отправка датаграммы (IPv4)

```c
net_socket_t* sock = NULL;

// Создаём UDP сокет
net_socket_create(NET_AF_INET4, NET_PROTO_UDP, NET_SOCK_TYPE_UDP, &sock);

// Привязываем к любому интерфейсу (опционально)
net_address_t local;
net_address_parse("0.0.0.0", NET_AF_INET4, &remote);
net_socket_bind(sock, &local);

// Устанавливаем адрес получателя
net_address_t remote;
net_address_parse("192.168.1.100", NET_AF_INET4, &remote);
net_htons(9003, remote.port);
net_socket_connect(sock, &remote);

// Отправляем
size_t sent = 0;
net_socket_send(sock, "Hello!", 6, &sent);

net_socket_close(sock);
```

### TCP — подключение и отправка (IPv6)

```c
net_socket_t* sock = NULL;

// Создаём TCP клиентский сокет
net_socket_create(NET_AF_INET6, NET_PROTO_TCP, NET_SOCK_TYPE_TCP_CONNECTION, &sock);

// Привязываем к любому интерфейсу (опционально)
net_address_t local;
net_address_parse("::", NET_AF_INET6, &local);
net_socket_bind(sock, &local);

// Через % указываем интерфес (default=0)
net_address_t remote;
net_address_parse("fe80::2889:bf2e:df6c:1e81%ens33", NET_AF_INET6, &remote);
net_htons(9001, remote.port);
net_socket_connect(sock, &remote);

// Отправляем
size_t sent = 0;
net_socket_send(sock, "Hello!", 6, &sent);

net_socket_close(sock);
```

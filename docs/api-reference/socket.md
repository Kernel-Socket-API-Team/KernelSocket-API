# Создание и управление сокетами

Данный раздел описывает основные функции для создания, закрытия и управления сокетами.

## Обзор функций 
```c
net_error_t net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type, net_socket_t** socketOut);

net_error_t net_socket_close(net_socket_t* sock);

net_error_t net_socket_bind(net_socket_t* sock, const net_address_t* addr);
```

## net_socket_create

```c
net_error_t net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type, net_socket_t** socketOut);
```

### Описание

Функция создание нового сокета.

### Параметры

 * **[in]** `family` — семейство адресов (`NET_AF_INET4` / `NET_AF_INET6`)
 * **[in]** `protocol` — транспортный протокол (`NET_PROTO_TCP` / `NET_PROTO_UDP`)
 * **[in]** `type` — тип сокета (слушающий /клиентский / UDP)
 * **[out]** `socketOut` — указатель на созданный сокет

### Возвращаемые значения

 * `NET_SUCCESS` — сокет успешно создан
 * `NET_ERROR_INVALID_PARAM` — некорректные параметры
 * `NET_ERROR_NOT_REGISTER` — библиотека не зарегистрирована
 * `NET_ERROR_NOT_INITIALIZED` — библиотека не инициализирована
 * [Другие ошибки связанные с работой библиотек из раздела платформенных ошибок](./errors.md)

## net_socket_close

```c
net_error_t net_socket_close(net_socket_t* sock);
```

### Описание
Закрывает сокет и освобождает связанные с ним ресурсы.

**Важно!** Принудительное закрытие сокета во время параллельно выполняющихся сетевых операциях с этим же сокетом в общем случае нежелательно. Однако иногда такое поведение бывает полезным — например, при работе с серверами. Данный сценарий дополнительно описывается в [соответствующем разделе](./server.md).

### Параметры

* **[in]** `sock` — закрываемый сокет

### Возвращаемые значения

 * `NET_SUCCESS` — сокет успешно закрыт
 * `NET_ERROR_INVALID_PARAM` — некорректные параметры
 * `NET_ERROR_INVALID_STATE` — сокет в невалидном состоянии

## net_socket_bind
```c
net_error_t net_socket_bind(net_socket_t* sock, const net_address_t* addr);
```

### Описание

Привязка сокета к адресу. Привязывает сокет к указанной структре адреса.

### Параметры

 * **[in]** `sock` — сокет для привязки
 * **[in]** `addr` — настроенный адрес

### Возвращаемые значения

 * `NET_SUCCESS` — успешная привязка
 * `NET_ERROR_INVALID_PARAM` — некорректные параметры
 * `NET_ERROR_INVALID_STATE` — сокет в неподходящем состоянии
 * [Другие ошибки связанные с работой библиотек из раздела платформенных ошибок](./errors.md)

## Примеры использования
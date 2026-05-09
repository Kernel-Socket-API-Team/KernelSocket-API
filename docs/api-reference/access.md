# Доступ к данным сокета

Данный раздел описывает access-функции (геттеры) для получения информации о сокете. Эти функции позволяют получить локальный и удалённый адреса, тип сокета, протокол, а также информацию об ошибках.

```c
net_error_t net_socket_get_address(net_socket_t* sock, net_address_t* addr);

net_error_t net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr);

net_error_t net_socket_get_type(net_socket_t* sock, net_socket_type_t* type);

net_error_t net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol);

net_error_t net_socket_last_error(net_socket_t* sock, net_error_t* error);

net_error_t net_socket_last_platform_error(net_socket_t* sock, const void** platform_error);
```

## `net_socket_get_address`
```c
net_error_t net_socket_get_address(net_socket_t* sock, net_address_t* addr);
```

### Описание

Получает локальный адрес сокета. Для не подключённых сокетов возвращает адрес, к которому привязан сокет (или неинициализированный адрес, если привязка не выполнялась).

### Параметры

* **[in]** `sock` — сокет
* **[out]** `addr` — структура для локального адреса

### Возвращаемые значения

* `NET_SUCCESS` — адрес успешно получен
* `NET_ERROR_INVALID_PARAM` — некорректные параметры (указатель на сокет или addr равен NULL)

## `net_socket_get_remote_address`
```c
net_error_t net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr);
```

### Описание

Получает удалённый адрес сокета. Для TCP-сокетов возвращает адрес подключённого узла. Для UDP-сокетов адрес доступен только если сокет был подключён с помощью `net_socket_connect`.

### Параметры

* **[in]** `sock` — сокет
* **[out]** `addr` — структура для удалённого адреса

### Возвращаемые значения

* `NET_SUCCESS` — адрес успешно получен
* `NET_ERROR_INVALID_PARAM` — некорректные параметры (указатель на сокет или addr равен NULL)

## `net_socket_get_type`
```c
net_error_t net_socket_get_type(net_socket_t* sock, net_socket_type_t* type);
```

### Описание
Получает тип сокета. Тип сокета определяется при его создании и не может быть изменён в процессе работы.

### Параметры
* **[in]** `sock` — сокет
* **[out]** `type` — тип сокета

### Возвращаемые значения

* `NET_SUCCESS` — тип успешно получен
* `NET_ERROR_INVALID_PARAM` — некорректные параметры (указатель на сокет или `type` равен `NULL`)

## `net_socket_get_protocol`
```c
net_error_t net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol);
```

### Описание
Получает протокол сокета. Протокол определяется при создании сокета (`NET_PROTO_TCP` или `NET_PROTO_UDP`).

### Параметры

* **[in]** `sock` — сокет
* **[out]** `protocol` — протокол сокета

### Возвращаемые значения

* `NET_SUCCESS` — протокол успешно получен

* `NET_ERROR_INVALID_PARAM` — некорректные параметры (указатель на сокет или protocol равен NULL)

## `net_socket_last_error`
```c
net_error_t net_socket_last_error(net_socket_t* sock, net_error_t* error);
```

### Описание

Получает последнюю ошибку, произошедшую при работе с сокетом. Ошибка сохраняется внутри структуры сокета и может быть прочитана многократно. Функция полезна для получения детальной информации об ошибке после того, как другая функция API вернула код ошибки.

### Параметры

* **[in]** `sock` — сокет
* **[out]** `error` — код последней ошибки

### Возвращаемые значения

* `NET_SUCCESS` — ошибка успешно получена
* `NET_ERROR_INVALID_PARAM` — некорректные параметры (указатель на сокет или error равен NULL)

## `net_socket_last_platform_error`
```c
net_error_t net_socket_last_platform_error(net_socket_t* sock, const void** platform_error);
```

### Описание
Получает платформозависимый код ошибки операционной системы. Функция предназначена для глубокой отладки и анализа низкоуровневых проблем, которые не могут быть описаны через стандартные коды ошибок библиотеки.

Пользователь должен привести полученный указатель к нужному типу в зависимости от платформы:

* Windows: `NTSTATUS*` — код ошибки Windows
* Linux: `int*` — код ошибки errno

### Параметры

* **[in]** `sock` — сокет
* **[out]** `platform_error` — указатель на платформозависимую ошибку

### Возвращаемые значения

* `NET_SUCCESS` — ошибка успешно получена
* `NET_ERROR_INVALID_PARAM` — некорректные параметры (указатель на сокет или platform_error равен NULL)

## Примеры использования

### Получение локального и удалённого адресов
```c
net_socket_t* sock;
net_address_t local_addr;
net_address_t remote_addr;
char buffer[NET_ADDRSTRLEN];

// Создание и подключение сокета...
// ...

// Получение локального адреса
if (net_socket_get_address(sock, &local_addr) == NET_SUCCESS) {
    net_address_to_string(&local_addr, buffer, sizeof(buffer), true);
}

// Получение удалённого адреса (для подключённого сокета)
if (net_socket_get_remote_address(sock, &remote_addr) == NET_SUCCESS) {
    net_address_to_string(&remote_addr, buffer, sizeof(buffer), true);
}
```

### Получение информации о типе и протоколе
```c
net_socket_type_t type;
net_protocol_t protocol;

if (net_socket_get_type(sock, &type) == NET_SUCCESS) {
    if (type == NET_SOCK_TYPE_TCP_LISTEN) {
        // Слушающий TCP сокет (сервер)
    } else if (type == NET_SOCK_TYPE_TCP_CONNECTION) {
        // Подключенный TCP сокет (клиент)
    } else if (type == NET_SOCK_TYPE_UDP) {
        // UDP сокет
    }
}

if (net_socket_get_protocol(sock, &protocol) == NET_SUCCESS) {
    if (protocol == NET_PROTO_TCP) {
        // Протокол TCP
    } else if (protocol == NET_PROTO_UDP) {
        // Протокол UDP
    }
}
```

### Диагностика ошибок
```c
net_error_t err = net_socket_send(sock, data, len, 0);

if (err != NET_SUCCESS) {
    net_error_t last_err;
    const void* platform_err;
    
    // Получение кода ошибки библиотеки
    net_socket_last_error(sock, &last_err);
    // last_err содержит код ошибки библиотеки
    
    // Получение платформозависимой ошибки для детальной диагностики
    if (net_socket_last_platform_error(sock, &platform_err) == NET_SUCCESS) {
        #ifdef _WIN32
                NTSTATUS status = *(NTSTATUS*)platform_err;
        #else
                int errno_val = *(int*)platform_err;
        #endif
    }
}
```
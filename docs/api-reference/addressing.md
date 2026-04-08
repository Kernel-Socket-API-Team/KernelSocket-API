# Работа с адресами

Данный раздел описывает функции для:

- Преобразования IPv4/IPv6 адресов в сетевой формат;
- Преобразования адресов обратно в строковое представление;
- Перевода значений портов между хостовым и сетевым порядком байт (Big-endian).

## Обзор функций

```c
net_error_t net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr);

net_error_t net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port);

net_error_t net_htons(uint16_t hostshort, uint16_t* netshort);

net_error_t net_ntohs(uint16_t netshort, uint16_t* hostshort);
```

## `net_address_parse`
```c
net_error_t net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr);
```
### Описание

Преобразует строковое представление IP-адреса в структуру net_address_t.

### Параметры
* **[in]** `str` - строка с адресом (например: "192.168.1.1" или "::1")
* **[in]** `ip_family` - семейство адресов: `NET_AF_INET4`/
`NET_AF_INET6`
* **[out]** `addr` - указатель на структуру, в которую будет записан результат

### Возвращаемые значения
* `NET_SUCCESS` — адрес успешно разобран
* `NET_ERROR_INVALID_PARAM` — некорректный формат адреса
* [Другие ошибки связанные с работой библиотек из раздела платформенных ошибок](./errors.md)


## `net_address_to_string`

```c
net_error_t net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port);
```

### Описание

Преобразует структуру `net_address_t` в строковое представление. Для IPv6 scope ID не поддерживается.
При включённом порте формат: `IP:PORT`. Максимальная длина строки для IPv6 с портом — 54 символа. Рекомендуется использовать буфер размера `NET_ADDRSTRLEN` (определен в `ksockapi.h`).

### Параметры
* **[in]** `addr` - cтруктура адреса
* **[out]** `buffer` - буфер для записи строки
* **[in]** `buffer_size` - размер буфера (рекомендуется `NET_ADDRSTRLEN`)
* **[in]** `include_port` - `true` — включить порт, `false` — только IP-адрес

### Возвращаемые значения
* `NET_SUCCESS` — успешное преобразование
* `NET_ERROR_INVALID_PARAM` — некорректные параметры
* `NET_ERROR_BUFFER_TOO_SMALL` — буфер слишком мал
* [Другие ошибки связанные с работой библиотек из раздела платформенных ошибок](./errors.md)

## net_htons
```c
net_error_t net_htons(uint16_t hostshort, uint16_t* netshort);
```
### Описание

Преобразует значение порта из хостового порядка байт в сетевой (Big-endian).

### Параметры
* **[in]** `hostshort` - порт в хостовом порядке байт
* **[out]** `netshort` - порт в сетевом порядке байт

### Возвращаемые значения
* `NET_SUCCESS` — успешное преобразование
* `NET_ERROR_INVALID_PARAM` — не передан указатель для записи результата

## net_ntohs
```c
net_error_t net_ntohs(uint16_t netshort, uint16_t* hostshort);
```
### Описание

Преобразует значение порта из сетевого порядка байт в хостовый.

### Параметры
* **[in]** `netshort` - порт в сетевом порядке байт
* **[out]** `hostshort` - порт в хостовом порядке байт

### Возвращаемые значения

* `NET_SUCCESS` — успешное преобразование
* `NET_ERROR_INVALID_PARAM` — не передан указатель для записи результата

## Примеры использования

### Преобразование портов 
```c
net_error_t err;
uint16_t port_host = 4444;
uint16_t port_net;
uint16_t port_back;

// Преобразование хостового порта в сетевой порядок (big-endian)
err = net_htons(port_host, &port_net);
// port_net = 0x5C11 (4444 в big-endian)

// Преобразование обратно из сетевого в хостовый
err = net_ntohs(port_net, &port_back);
// port_back = 4444
```

### Разбор IP-адресов

```c
net_address_t addr;

// Разбор IPv4 адреса
net_address_parse("192.168.1.1", NET_AF_INET4, &addr);
// Представление в сетевом порядке находится в addr.addr.ipv4

// Разбор IPv6 адреса
net_address_parse("2001:db8::1", NET_AF_INET6, &addr);
// Представление в сетевом порядке находится в addr.addr.ipv6
```

### Преобразование адреса в строку
```c
net_address_t addr;
char buffer[NET_ADDRSTRLEN];

// Заполнение адреса (например, после accept или receive)
// ...

// Преобразование в строку без порта
net_address_to_string(&addr, buffer, sizeof(buffer), 0);
// buffer = "192.168.1.1" или "2001:db8::1"

// Преобразование в строку с портом
net_address_to_string(&addr, buffer, sizeof(buffer), 1);
// buffer = "192.168.1.1:4444" или "[2001:db8::1]:4444"
```

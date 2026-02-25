#ifndef NETWORK_API_H
#define NETWORK_API_H

/**
 * ksockapi.h
 * Главный заголовочный файл Kernel Socket API
 * 
 * Этот файл объединяет все публичные интерфейсы библиотеки.
 * Для использования API достаточно включить этот файл.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * ======================================
 * Константы и типы возвращаемых ошибок
 * ======================================
 */

// Коды возврата
typedef enum {
    NET_SUCCESS = 0,
    NET_ERROR_GENERIC = -1,
    NET_ERROR_INVALID_PARAM = -2,
	/* Количество кодов возрастет в дальнейшем, их необходимость 
	важно обсудить с разработчиками */
} net_error_t;

/* 
 * ======================================
 * Типы протоколов и адресации
 * ======================================
 */

// Типы протоколов
typedef enum {
    NET_PROTO_TCP = 1,
    NET_PROTO_UDP = 2
} net_protocol_t;

// Типы адресов (IPv4/IPv6)
typedef enum {
    NET_AF_INET4 = 2,   /* IPv4 */
    NET_AF_INET6 = 10  /* IPv6 */
} net_family_t;

// Флаги для неблокирующих операций
typedef enum {
    NET_FLAG_NONE = 0,
    NET_FLAG_NONBLOCK = 1,      /* Неблокирующий режим */
    NET_FLAG_REUSEADDR = 2,     /* Переиспользовать адрес */
    NET_FLAG_BROADCAST = 4,     /* Разрешить широковещательные пакеты (UDP) */
    NET_FLAG_KEEPALIVE = 8      /* Поддерживать соединение активным (TCP) */
} net_flags_t;

/* 
 * ======================================
 * Структуры данных
 * ======================================
 */

// Дескриптор сокета (абстрактный тип)
typedef struct net_socket net_socket_t;

// Адрес сокета (универсальное представление)
typedef struct net_address {
    net_family_t family;         // Семейство адресов (IPv4/IPv6) 
    uint16_t port;               // Порт (в сетевом порядке байт)
    union {
        uint32_t ipv4;           //  IPv4 адрес (в сетевом порядке байт)
        uint8_t ipv6[16];        //  IPv6 адрес
    } addr;
    char hostname[256];          // Человекочитаемое имя (опционально)
} net_address_t;

// Параметры сокета
typedef struct net_socket_options {
    int recv_buffer_size;         // Размер буфера приема
    int send_buffer_size;         // Размер буфера отправки
    int recv_timeout_ms;          // Таймаут приема (мс)
    int send_timeout_ms;          // Таймаут отправки (мс)
    int ttl;                      // Time To Live
    int broadcast;                // Разрешить broadcast (0/1)
    int keepalive;                // Использовать keepalive (0/1)
} net_socket_options_t;

/* 
 * ======================================
 * Основные функции API
 * ======================================
 */

// ----- Инициализация и завершение работы -----

// Инициализация библиотеки (вызывается один раз при старте)
net_error_t net_initialize(void);

// Завершение работы библиотеки (освобождение ресурсов)
net_error_t net_cleanup(void);

// ----- Создание и управление сокетами -----

/*
 * Создание сокета
 * family - Семейство протоколов (IPv4/IPv6)
 * protocol - Тип протокола (TCP/UDP)
 * flags - Флаги (NET_FLAG_*)
 * return net_socket_t* - Указатель на созданный сокет или NULL при ошибке
 */
net_socket_t* net_socket_create(net_family_t family, net_protocol_t protocol, int flags);

// Закрытие сокета и освобождение ресурсов
net_error_t net_socket_close(net_socket_t* sock);

// Установка параметров сокета
net_error_t net_socket_set_options(net_socket_t* sock, const net_socket_options_t* opts);

// Получение параметров сокета
net_error_t net_socket_get_options(net_socket_t* sock, net_socket_options_t* opts);

// ----- Привязка и установка соединения -----

// Привязка сокета к локальному адресу
net_error_t net_socket_bind(net_socket_t* sock, const net_address_t* addr);

// Установка соединения (для TCP-клиентов)
net_error_t net_socket_connect(net_socket_t* sock, const net_address_t* addr);

/*
 * Перевод сокета в режим прослушивания (для TCP-сервера)
 * backlog - Максимальная длина очереди ожидающих соединений
 */
net_error_t net_socket_listen(net_socket_t* sock, int backlog);

/*
 * Принятие входящего соединения (для TCP-сервера)
 * sock - Слушающий сокет
 * client_addr [out] - Адрес клиента (может быть NULL)
 * return net_socket_t* - Новый сокет для общения с клиентом или NULL при ошибке
 */
net_socket_t* net_socket_accept(net_socket_t* sock, net_address_t* client_addr);

// ----- Отправка и прием данных -----

/*
 * Отправка данных (для TCP и UDP)
 * data - Указатель на данные
 * size - Размер данных в байтах
 * sent [out] - Количество реально отправленных байт (может быть NULL)
 */
net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);

// Отправка данных с указанием адреса назначения (для UDP)
net_error_t net_socket_send_to(net_socket_t* sock, const void* data, size_t size, const net_address_t* dest_addr, size_t* sent);

/*
 * Прием данных (для TCP и UDP)
 * buffer - Буфер для приема данных
 * buffer_size - Размер буфера
 * received [out] - Количество реально принятых байт
 */
net_error_t net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, size_t* received);

/*
 * Прием данных с получением адреса отправителя (для UDP)
 */
net_error_t net_socket_receive_from(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* src_addr, size_t* received);

/* ----- Вспомогательные функции ----- */

/*
 * Преобразование строкового адреса в структуру net_address_t
 * str - Адрес в виде строки (например, "192.168.1.1" или "::1")
 * default_port - Порт по умолчанию (если например не указан в адресе)
 */
net_error_t net_address_parse(const char* str, uint16_t default_port, net_address_t* addr);

// Преобразование структуры net_address_t в строку
const char* net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size);

// Получение локального адреса сокета
net_error_t net_socket_get_local_address(net_socket_t* sock, net_address_t* addr);

// Получение удаленного адреса сокета
net_error_t net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr);

// Установка неблокирующего режима
net_error_t net_socket_set_nonblocking(net_socket_t* sock, int enable);

// Проверка, есть ли данные для чтения (опционально)
net_error_t net_socket_can_read(net_socket_t* sock, int timeout_ms, int* can_read);

// Проверка, можно ли записывать данные (опционально)
net_error_t net_socket_can_write(net_socket_t* sock, int timeout_ms, int* can_write);

// ----- Функции получения последней ошибки -----

// Получение текстового описания последней ошибки для данного сокета
const char* net_socket_last_error(net_socket_t* sock);

// Получение текстового описания кода ошибки
const char* net_error_string(net_error_t err);

#ifdef __cplusplus
}
#endif

#endif NETWORK_API_H 
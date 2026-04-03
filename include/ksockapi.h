#ifndef NETWORK_API_H
#define NETWORK_API_H

/*
 * ksockapi.h
 * Главный заголовочный файл Kernel Socket API
 * 
 * Этот файл объединяет все публичные интерфейсы библиотеки.
 * Для использования API достаточно включить этот файл.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32    // Windows kernel type
#include <ntddk.h>

// стандартные типы в Windows Kernel
typedef UCHAR   uint8_t;
typedef USHORT  uint16_t;
typedef ULONG   uint32_t;
typedef ULONGLONG uint64_t;

typedef CHAR    int8_t;
typedef SHORT   int16_t;
typedef LONG    int32_t;
typedef LONGLONG int64_t;

typedef SIZE_T  size_t;
typedef SSIZE_T ptrdiff_t;

typedef BOOLEAN bool;

#else // Linux kernel type
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/stddef.h>

#endif

/* 
 * ======================================
 * Константы и типы возвращаемых ошибок
 * ======================================
 */

// Коды возврата
typedef enum {
    NET_ERROR_NOT_INITIALIZED = 0,      // Библиотека не инициализирована
    NET_ERROR_NOT_REGISTER = -1,
    NET_ERROR_INVALID_VTABLE = -2,      // Некорректная виртуальная таблица
    
    NET_SUCCESS = -3,                   // Успешная работа функции
    NET_ERROR_NO_MEMORY = -4,           // Ошибка работы с памятью
    NET_ERROR_ACCESS_DENIED = -5,       // Ошибка прав доступа
    NET_ERROR_TIMEOUT = -6,             // Превышено время ожидания операции
    NET_ERROR_BUFFER_TOO_SMALL = -7,    // Размер буфера слишком малл
    NET_ERROR_INVALID_PARAM = -8,       // Неправильные параметры
    NET_ERROR_GENERIC = -9,             // Общая (неизвестная) ошибка

    NET_ERROR_INVALID_STATE = -10,      // Ошибка получения контекста
    NET_ERROR_INVALID_PROTOCOL = -11,   // Передача не того протокола в функцию

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
    NET_AF_INET4 = 2,  /* IPv4 */
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

// Универсальная длина буффера для перевода адреса в строкове представление
#define NET_ADDRSTRLEN 54

// Бесконечное ожидание загрузки
#define NET_WAIT_INFINITE ((size_t)-1)

/* 
 * ======================================
 * Основные функции API
 * ======================================
 */

// ----- Инициализация и завершение работы -----

// Инициализация библиотеки (вызывается один раз при старте)
net_error_t net_register(void);

// Проверяем готовность библиотеки (вызывается в рабочем потоке)
// При передаче 0 ждем до момента инициализации (эмитация INFINITY)
// NET_SUCCESS в случае успеха и NET_ERROR_TIMEOUT в противном случае
net_error_t net_activate(const size_t limitMS);

// Проверяем готовность библиотеки к использованию
// вовзращает NET_SUCCESS в случае успешной инициализации и
// возвращает NET_ERROR_NOT_INITIALIZED
net_error_t net_is_ready(void);

// Завершение работы библиотеки (освобождение ресурсов)
net_error_t net_cleanup(void);

// ----- Создание и управление сокетами -----

/*
 * Создание сокета
 * family - Семейство протоколов (IPv4/IPv6)
 * protocol - Тип протокола (TCP/UDP)
 * flags - Флаги (NET_FLAG_*)
 * socketOut - Указатель на созданный сокет
 */
net_error_t net_socket_create(net_family_t family, net_protocol_t protocol, int flags, net_socket_t* socketOut);

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

// ----- Отправка и прием данных -----

/*
 * Отправка данных (для TCP и UDP)
 * data - Указатель на данные
 * size - Размер данных в байтах
 * sent [out] - Количество реально отправленных байт (может быть NULL)
 */
net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);

/*
 * sock        - Сокет (TCP listen, TCP connected или UDP)
 * peer_sock   - [OUT] Для TCP listen: новый клиентский сокет
 *                     Для TCP connected: NULL или тот же сокет
 *                     Для UDP: NULL
 * buffer      - [OUT] Буфер для данных
 * buffer_size - Размер буфера
 * from_addr   - [OUT] Адрес отправителя (для UDP) или клиента (для TCP)
 * received    - [OUT] Количество реально принятых байт
 */
net_error_t net_socket_receive(net_socket_t* sock, net_socket_t** peer_sock, void* buffer, size_t buffer_size, net_address_t* from_addr, size_t* received);

// Переводит TCP сокет в режим прослушивания входящих соединений
net_error_t net_socket_listen(net_socket_t* sock, int backlog);

/* ----- Вспомогательные функции ----- */

/*
 * Преобразование строкового адреса в структуру net_address_t
 * str - Адрес в виде строки (например, "192.168.1.1" или "::1")
 */
net_error_t net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr);

// Преобразование 16-битного значения (порт) из хостового в сетевой порядок
net_error_t net_htons(uint16_t hostshort, uint16_t* netshort);

// Преобразование 16-битного значения (порт) из сетевого в хостовой порядок
net_error_t net_ntohs(uint16_t netshort, uint16_t* hostshort);

// Преобразование структуры net_address_t в строку
net_error_t net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port);

// Получение локального адреса сокета
net_error_t net_socket_get_local_address(net_socket_t* sock, net_address_t* addr);

// Получение удаленного адреса сокета
net_error_t net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr);

// Установка неблокирующего режима
net_error_t net_socket_set_nonblocking(net_socket_t* sock, int enable);

// ----- Функции получения последней ошибки -----

// Получение последней ошибки для данного сокета
net_error_t net_socket_last_error(net_socket_t* sock, net_error_t error);

// Получение последние ошибки типа const void*
net_error_t net_socket_last_platform_error(net_socket_t* sock, const void* platform_error);

#ifdef __cplusplus
}
#endif

#endif
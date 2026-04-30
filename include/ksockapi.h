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
extern "C"
{
#endif

#ifdef _WIN32

#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdm.h>
#include <ip2string.h>
#include <netioapi.h>
#include <ws2ipdef.h>
#include <wsk.h>

    typedef UCHAR uint8_t;
    typedef USHORT uint16_t;
    typedef ULONG uint32_t;
    typedef ULONGLONG uint64_t;
    typedef CHAR int8_t;
    typedef SHORT int16_t;
    typedef LONG int32_t;
    typedef LONGLONG int64_t;
    typedef SIZE_T size_t;
    typedef SSIZE_T ptrdiff_t;
    typedef BOOLEAN bool;

#else
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/types.h>
#endif

    /*
     * ======================================
     * Константы и типы возвращаемых ошибок
     * ======================================
     */

    /* Коды ошибок Network API */
    typedef enum
    {

        /* Ошибки инициализации библиотеки */
        NET_ERROR_NOT_INITIALIZED = 0, // Библиотека не инициализирована
        NET_ERROR_NOT_REGISTER = -1,   // Библиотека не зарегистрирована
        NET_ERROR_INVALID_VTABLE = -2, // Некорректная виртуальная таблица
        NET_SUCCESS = -3,              // Успешное выполнение операции

        /* Платформенные ошибки */
        NET_ERROR_NO_MEMORY = -4,        // Недостаточно памяти
        NET_ERROR_ACCESS_DENIED = -5,    // Доступ запрещен
        NET_ERROR_TIMEOUT = -6,          // Превышено время ожидания
        NET_ERROR_BUFFER_TOO_SMALL = -7, // Размер буфера слишком мал
        NET_ERROR_INVALID_PARAM = -8,    // Некорректный параметр
        NET_ERROR_GENERIC = -9,          // Общая ошибка

        /* Ошибки работы с сокетами */
        NET_ERROR_INVALID_STATE = -10,   // Сокет в невалидном состоянии
        NET_ERROR_INVALID_PROTOCOL = -11 // Некорректный протокол

    } net_error_t;

    /*
     * ======================================
     * Типы протоколов и адресации
     * ======================================
     */

    /* Типы транспортных протоколов */
    typedef enum
    {
        NET_PROTO_TCP = 1, // TCP протокол
        NET_PROTO_UDP = 2  // UDP протокол
    } net_protocol_t;

    /* Семейства адресов (типы IP) */
    typedef enum
    {
        NET_AF_INET4 = 4, // IPv4
        NET_AF_INET6 = 6  // IPv6
    } net_family_t;

    /* Типы сокетов */
    typedef enum
    {
        NET_SOCK_TYPE_TCP_LISTEN,     // Слушающий TCP сокет (сервер)
        NET_SOCK_TYPE_TCP_CONNECTION, // Подключенный TCP сокет (клиент)
        NET_SOCK_TYPE_UDP             // UDP сокет
    } net_socket_type_t;

    /*
     * ======================================
     * Структуры данных
     * ======================================
     */

    /*
     * Дескриптор сокета (абстрактный тип)
     *
     * Содержит внутреннее состояние сокета. Пользователь не должен
     * обращаться к полям напрямую. Используйте функции API.
     */
    typedef struct net_socket net_socket_t;

    /*
     * Структура сетевого адреса
     *
     * Универсальное представление адреса для IPv4 и IPv6.
     * Все данные хранятся в сетевом порядке байт. Для перевода
     * в сетевой порядок и обратно API предоставляет
     * соответствующие функции.
     */
    typedef struct net_address
    {
        net_family_t family; // Семейство адресов (IPv4/IPv6)
        uint16_t port;       // Порт (в сетевом порядке байт)
        union
        {
            uint32_t ipv4;    // IPv4 адрес (в сетевом порядке)
            uint8_t ipv6[16]; // IPv6 адрес (в сетевом порядке)
        } addr;
        uint32_t scope_id; // Указание индекса сетевого интерфейса для Link-Local адресов
        char hostname[256]; // Человекочитаемое имя (опционально)
    } net_address_t;

#define NET_ADDRSTRLEN 64                // Максимальная длина строкового адреса
#define NET_WAIT_INFINITE ((size_t) - 1) // Бесконечное ожидание

    /*
     * ======================================
     * Инициализация и завершение работы
     * ======================================
     */

    /*
     * Регистрация библиотеки в системе
     *
     * Должна быть вызвана один раз при загрузке драйвера.
     * При повторном вызове возвращает NET_SUCCESS.
     *
     * @return NET_SUCCESS                 - успешная регистрация
     * @return NET_ERROR_GENERIC           - ошибка регистрации
     * @return (Другие платформенные ошибки из net_error_t)
     */
    net_error_t net_register(void);

    /*
     * Активация библиотеки и ожидание готовности WSK
     *
     * Вызывается в рабочем потоке. Может блокировать выполнение потока
     * до готовности подсистемы или истечения таймаута.
     *
     * @param[in] limitMS - максимальное время ожидания в мс,
     *                      NET_WAIT_INFINITE для бесконечного ожидания,
     *                      0 для неблокирующей проверки
     *
     * @return NET_SUCCESS                  - библиотека готова
     * @return NET_ERROR_TIMEOUT            - превышено время ожидания
     * @return NET_ERROR_NOT_REGISTER       - библиотека не зарегистрирована
     * @return (Другие платформенные ошибки из net_error_t)
     */
    net_error_t net_activate(size_t limitMS);

    /*
     * Проверка готовности библиотеки
     *
     * Неблокирующая проверка состояния библиотеки.
     *
     * @return NET_SUCCESS                  - библиотека готова
     * @return NET_ERROR_NOT_REGISTER       - библиотека не зарегистрирована
     * @return NET_ERROR_NOT_INITIALIZED    - библиотека не инициализирована
     */
    net_error_t net_is_ready(void);

    /*
     * Завершение работы и освобождение ресурсов
     *
     * Должна быть вызвана при выгрузке драйвера после завершения всех
     * операций с сокетами. Также вызывается в случае ошибки активации.
     *
     * @return NET_SUCCESS              - успешное завершение
     * @return NET_ERROR_NOT_REGISTER   - библиотека не зарегистрирована
     */
    net_error_t net_cleanup(void);

    /*
     * ======================================
     * Создание и управление сокетами
     * ======================================
     */

    /*
     * Создание нового сокета
     *
     * @param[in]   family      - семейство адресов (NET_AF_INET4/NET_AF_INET6)
     * @param[in]   protocol    - транспортный протокол (NET_PROTO_TCP/NET_PROTO_UDP)
     * @param[in]   type        - тип сокета (слушающий/клиентский/UDP)
     * @param[out]  socketOut   - указатель на созданный сокет
     *
     * @return NET_SUCCESS                  - сокет успешно создан
     * @return NET_ERROR_INVALID_PARAM      - некорректные параметры
     * @return NET_ERROR_NOT_REGISTER       - библиотека не зарегистрирована
     * @return NET_ERROR_NOT_INITIALIZED    - библиотека не инициализирована
     * @return (Другие платформенные ошибки из net_error_t)
     */
    net_error_t net_socket_create(net_family_t family, net_protocol_t protocol, net_socket_type_t type,
                                  net_socket_t** socketOut);

    /*
     * Закрытие сокета и освобождение ресурсов
     *
     * Закрывает сокет, прерывает все ожидающие операции и освобождает
     * связанную с ним память.
     *
     * @param[in] sock - закрываемый сокет
     *
     * @return NET_SUCCESS              - сокет успешно закрыт
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     * @return NET_ERROR_INVALID_STATE  - сокет в невалидном состоянии
     */
    net_error_t net_socket_close(net_socket_t* sock);

    /*
     * ======================================
     * Привязка и установка соединения
     * ======================================
     */

    /*
     * Привязка сокета к адресу.
     *
     * Привязывает сокет к указанной структре адреса.
     *
     * @param[in] sock - сокет для привязки
     * @param[in] addr - настроенный адрес
     *
     * @return NET_SUCCESS              - успешная привязка
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     * @return NET_ERROR_INVALID_STATE  - сокет в неподходящем состоянии
     * @return (Другие платформенные ошибки из net_error_t)
     */
    net_error_t net_socket_bind(net_socket_t* sock, const net_address_t* addr);

    /*
     * Установка TCP-соединения с удаленным хостом
     *
     * @param[in] sock - клиентский сокет
     * @param[in] addr - адрес удаленного сервера
     *
     * @return NET_SUCCESS                  - соединение установлено
     * @return NET_ERROR_INVALID_PARAM      - некорректные параметры
     * @return NET_ERROR_INVALID_PROTOCOL   - вызов для UDP сокета
     * @return NET_ERROR_TIMEOUT            - превышено время ожидания
     */
    net_error_t net_socket_connect(net_socket_t* sock, const net_address_t* addr);

    /*
     * ======================================
     * Отправка и прием данных
     * ======================================
     */

    /*
     * Отправка данных через сокет
     *
     * Для TCP: отправляет данные установленному клиенту.
     * Для UDP: требует предварительного вызова net_socket_connect
     *         или использует сохраненный адрес.
     *
     * @param[in]   sock    - сокет для отправки
     * @param[in]   data    - указатель на отправляемые данные
     * @param[in]   size    - размер данных в байтах
     * @param[out]  sent    - количество реально отправленных байт (может быть NULL)
     *
     * @return NET_SUCCESS              - данные отправлены
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     * @return NET_ERROR_INVALID_STATE  - сокет не готов к отправке
     */
    net_error_t net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);

    /*
     * Принятие входящего TCP-соединения
     *
     * Блокирует выполнение до установки нового соединения.
     * Возвращает новый сокет для общения с клиентом.
     * Адрес клиента (IPv4 или IPv6) автоматически сохраняется в сокете
     * и может быть получен через net_socket_get_address().
     *
     * @param[in]   server      - слушающий сокет (должен быть создан с NET_SOCK_TYPE_TCP_LISTEN)
     * @param[out]  client_out  - новый сокет для общения с клиентом
     *
     * @return NET_SUCCESS                  - новое соединение принято
     * @return NET_ERROR_INVALID_PARAM      - некорректные параметры (server или client_out == NULL)
     * @return NET_ERROR_INVALID_PROTOCOL   - server не является TCP сокетом
     * @return NET_ERROR_INVALID_STATE      - сокет не в режиме прослушивания или ошибка контекста
     * @return NET_ERROR_NO_MEMORY          - недостаточно памяти для нового сокета
     * @return (Другие платформенные ошибки из net_error_t)
     */
    net_error_t net_socket_accept(net_socket_t* server, net_socket_t** client_out);

    /*
     * Прием данных через сокет
     *
     * Для TCP сокета: читает данные от подключенного клиента.
     * Для UDP сокета: читает дейтаграмму и возвращает адрес отправителя.
     *
     * Данная функция НЕ выполняет автоматический accept для TCP listen сокетов.
     * Для TCP сервера необходимо сначала вызвать net_socket_accept() для получения
     * клиентского сокета, затем использовать этот сокет для вызова receive.
     *
     * @param[in]   sock        - сокет для приема (должен быть TCP connected или UDP)
     * @param[out]  buffer      - буфер для данных
     * @param[in]   buffer_size - размер буфера
     * @param[out]  from_addr   - адрес отправителя (для UDP) или NULL.
     *                            Для TCP сокета этот параметр можно игнорировать, адрес клиента
     *                            можно получить через net_socket_get_address() после accept.
     * @param[out]  received    - количество реально принятых байт
     *
     * @return NET_SUCCESS              - данные получены
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры (sock, buffer или buffer_size)
     * @return NET_ERROR_INVALID_STATE  - сокет не готов к приему данных
     * @return NET_ERROR_NO_MEMORY      - недостаточно памяти для буфера или IRP
     * @return (Другие платформенные ошибки из net_error_t)
     */
    net_error_t net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, net_address_t* from_addr,
                                   size_t* received);

    /*
     * ======================================
     * Вспомогательные функции
     * ======================================
     */

    /*
     * Преобразование строки в структуру адреса
     *
     * @param[in] str       - строковое представление адреса ("192.168.1.1" или "::1")
     * @param[in] ip_family - семейство адресов (NET_AF_INET4/NET_AF_INET6)
     * @param[out] addr     - структура для заполнения
     *
     * @return NET_SUCCESS              - адрес успешно разобран
     * @return NET_ERROR_INVALID_PARAM  - некорректный формат адреса
     * @return (Другие платформенные ошибки из net_error_t)
     */
    net_error_t net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr);

    /*
     * Преобразование порта из хостового в сетевой порядок байт
     *
     * @param[in] hostshort - порт в хостовом порядке
     * @param[out] netshort - порт в сетевом порядке
     *
     * @return NET_SUCCESS              - успешное преобразование
     * @return NET_ERROR_INVALID_PARAM  - не передается адрес для записи порта
     */
    net_error_t net_htons(uint16_t hostshort, uint16_t* netshort);

    /*
     * Преобразование порта из сетевого в хостовый порядок байт
     *
     * @param[in] netshort      - порт в сетевом порядке
     * @param[out] hostshort    - порт в хостовом порядке
     *
     * @return NET_SUCCESS              - успешное преобразование
     * @return NET_ERROR_INVALID_PARAM  - не передается адрес для записи порта
     */
    net_error_t net_ntohs(uint16_t netshort, uint16_t* hostshort);

    /*
     * Преобразование структуры адреса в строку
     *
     * Преобразует структуру net_address_t (IPv4 или IPv6) в человекочитаемую строку.
     *
     * @param[in] addr          - структура адреса
     * @param[out] buffer       - буфер для строки
     * @param[in] buffer_size   - размер буфера (рекомендуется NET_ADDRSTRLEN)
     * @param[in] include_port  - TRUE: включить порт в формате "IP:PORT",
     *                            FALSE: только IP адрес
     *
     * @return NET_SUCCESS                  - строка успешно сформирована
     * @return NET_ERROR_INVALID_PARAM      - некорректные параметры
     * @return NET_ERROR_BUFFER_TOO_SMALL   - буфер слишком мал для адреса
     *
     * Для IPv6 адресов максимальная длина строки с портом - 54 символа
     * (NET_ADDRSTRLEN). Рекомендуется использовать буфер этого размера.
     */
    net_error_t net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port);

    /*
     * Получение адреса сокета
     *
     * @param[in] sock  - сокет
     * @param[out] addr - структура для адреса
     *
     * @return NET_SUCCESS              - адрес получен
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     */
    net_error_t net_socket_get_address(net_socket_t* sock, net_address_t* addr);

    /*
     * Получение типа сокета
     *
     * @param[in] sock  - сокет
     * @param[out] type - тип сокета
     *
     * @return NET_SUCCESS              - тип получен
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     */
    net_error_t net_socket_get_type(net_socket_t* sock, net_socket_type_t* type);

    /*
     * Получение протокола сокета
     *
     * @param[in] sock      - сокет
     * @param[out] protocol - протокол (TCP/UDP)
     *
     * @return NET_SUCCESS              - протокол получен
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     */
    net_error_t net_socket_get_protocol(net_socket_t* sock, net_protocol_t* protocol);

    /*
     * Получение последней ошибки для сокета
     *
     * @param[in] sock      - сокет
     * @param[out] error    - код ошибки
     *
     * @return NET_SUCCESS              - ошибка получена
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     */
    net_error_t net_socket_last_error(net_socket_t* sock, net_error_t* error);

    /*
     * Получение платформозависимой ошибки
     *
     * @param[in] sock              - сокет
     * @param[out] platform_error   - указатель на ошибку ОС (NTSTATUS для Windows)
     *
     * @return NET_SUCCESS              - ошибка получена
     * @return NET_ERROR_INVALID_PARAM  - некорректные параметры
     *
     * Пользователь должен привести platform_error к нужному типу:
     * NTSTATUS* на Windows, int* на Linux.
     */
    net_error_t net_socket_last_platform_error(net_socket_t* sock, const void** platform_error);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_API_H */
#include "linux_common.h"

LINUX_CONTEXT g_LinuxContext = {
    .Initialized = false,
    .Activated = false
};

net_error_t convert_status_from_linux(int error, net_socket_t* sock)
{
    if (error >= 0)
    {
        if (sock)
        {
            sock->error = NET_SUCCESS;
            sock->last_error = (void*)(long)error;  // сохраняем 0 как успех
        }
        return NET_SUCCESS;
    }

    net_error_t net_error;
    int code = -error;  // Получаем положительный код ошибки

    switch (code)
    {
    case ENOMEM:
        net_error = NET_ERROR_NO_MEMORY;
        break;
    case EACCES:
    case EPERM:
        net_error = NET_ERROR_ACCESS_DENIED;
        break;
    case EINVAL:
        net_error = NET_ERROR_INVALID_PARAM;
        break;
    case ETIMEDOUT:
        net_error = NET_ERROR_TIMEOUT;
        break;
    case EMSGSIZE:
        net_error = NET_ERROR_BUFFER_TOO_SMALL;
        break;
    case ENOBUFS:
        net_error = NET_ERROR_NO_MEMORY;
        break;
    default:
        net_error = NET_ERROR_GENERIC;
        break;
    }

    if (sock)
    {
        sock->error = net_error;
        sock->last_error = (void*)(long)error;  // сохраняем оригинальный код ошибки
    }

    return net_error;
}

static uint32_t get_interface_index(const char* ifname)
{
    struct net_device *dev;
    uint32_t ifindex;
    
    if (!ifname || !*ifname)
        return 0;
    
    dev = dev_get_by_name(&init_net, ifname);
    if (!dev)
        return 0;
    
    ifindex = dev->ifindex;
    dev_put(dev);
    
    return ifindex;
}

net_error_t parse_ipv4_address(const char* str, net_address_t* addr)
{
    char work_str[64] = {0};
    char ip_part[16] = {0};
    char port_part[6] = {0};
    char* percent_ptr = NULL;
    char* port_ptr = NULL;
    
    // Копируем исходную строку
    strncpy(work_str, str, sizeof(work_str) - 1);
    
    // Сначала находим порт
    port_ptr = strrchr(work_str, ':');
    
    if (port_ptr) {
        // Сохраняем порт
        char* port_start = port_ptr + 1;
        size_t port_len = strlen(port_start);
        if (port_len > 0 && port_len < sizeof(port_part)) {
            memcpy(port_part, port_start, port_len + 1);
            
            // Обрезаем строку до порта (временно)
            *port_ptr = '\0';
        }
    }
    
    // Ищем и удаляем % в IP части
    percent_ptr = strchr(work_str, '%');
    if (percent_ptr) {
        // Вырезаем % и всё что после него до конца или до порта
        *percent_ptr = '\0';
    }
    
    // Копируем IP часть
    strncpy(ip_part, work_str, sizeof(ip_part) - 1);
    
    // Парсим IP
    __be32 ipv4_addr;
    if (in4_pton(ip_part, -1, (u8*)&ipv4_addr, -1, NULL) != 1) {
        return NET_ERROR_INVALID_PARAM;
    }
    
    // Заполняем структуру
    addr->family = NET_AF_INET4;
    addr->addr.ipv4 = ipv4_addr;
    addr->scope_id = 0;
    
    // Парсим порт если был
    if (port_ptr && port_part[0] != '\0') {
        unsigned long port_val = simple_strtoul(port_part, NULL, 10);
        if (port_val > 65535) {
            return NET_ERROR_INVALID_PARAM;
        }
        addr->port = htons((uint16_t)port_val);
    } else {
        addr->port = 0;
    }
    
    return NET_SUCCESS;
}

net_error_t parse_ipv6_address(const char* str, net_address_t* addr)
{
    char addr_buf[INET6_ADDRSTRLEN] = {0};
    char port_buf[6] = {0};
    char scope_buf[IFNAMSIZ] = {0};
    const char* ip_start = str;
    const char* ip_end = NULL;
    const char* port_ptr = NULL;
    bool has_port = false;
    bool has_brackets = false;
    
    // Обнуляем порт по умолчанию
    addr->port = 0;
    
    // Проверяем формат с квадратными скобками
    if (*str == '[') {
        has_brackets = true;
        ip_start = str + 1;
        ip_end = strchr(ip_start, ']');
        
        if (!ip_end) {
            return NET_ERROR_INVALID_PARAM;
        }
        
        // Проверяем наличие порта после скобок
        if (*(ip_end + 1) == ':') {
            has_port = true;
            port_ptr = ip_end + 2;
        }
        
        // Извлекаем IP часть
        size_t ip_len = ip_end - ip_start;
        if (ip_len == 0 || ip_len >= sizeof(addr_buf)) {
            return NET_ERROR_INVALID_PARAM;
        }
        memcpy(addr_buf, ip_start, ip_len);
        addr_buf[ip_len] = '\0';
    } else {
        // Без квадратных скобок
        has_brackets = false;
        
        // Копируем всю строку
        strncpy(addr_buf, str, sizeof(addr_buf) - 1);
        
        // Проверяем наличие порта (последнее двоеточие с цифрами после него)
        const char* last_colon = strrchr(addr_buf, ':');
        if (last_colon && last_colon > addr_buf) {
            // Проверяем, что после двоеточия только цифры (это порт)
            const char* port_check = last_colon + 1;
            bool is_port = true;
            for (const char* p = port_check; *p; p++) {
                if (*p < '0' || *p > '9') {
                    is_port = false;
                    break;
                }
            }
            
            // Также проверяем, что это не двойное двоеточие (::)
            if (is_port && *(last_colon - 1) != ':') {
                has_port = true;
                port_ptr = port_check;
                // Обрезаем строку до порта
                size_t addr_len = last_colon - addr_buf;
                addr_buf[addr_len] = '\0';
            }
        }
    }
    
    // Ищем scope_id в IP части
    char* scope_pos = strchr(addr_buf, '%');
    if (scope_pos) {
        *scope_pos = '\0';  // Обрезаем IP часть
        const char* scope_start = scope_pos + 1;
        
        // Копируем scope string
        size_t scope_len = strlen(scope_start);
        if (scope_len < sizeof(scope_buf)) {
            memcpy(scope_buf, scope_start, scope_len + 1);
        }
        
        // Парсим scope_id
        char* endptr;
        unsigned long scope_val = simple_strtoul(scope_buf, &endptr, 10);
        
        if (*endptr == '\0') {
            // Числовой scope_id
            addr->scope_id = (uint32_t)scope_val;
        } else {
            // Имя интерфейса
            addr->scope_id = get_interface_index(scope_buf);
            // Если интерфейс не найден, но это допустимо (может быть числовой scope)
            if (addr->scope_id == 0 && *scope_buf != '\0') {
                // Пробуем интерпретировать как число еще раз
                scope_val = simple_strtoul(scope_buf, &endptr, 10);
                if (*endptr == '\0') {
                    addr->scope_id = (uint32_t)scope_val;
                }
            }
        }
    } else {
        addr->scope_id = 0;
    }
    
    // Парсим порт если есть
    if (has_port && port_ptr && *port_ptr) {
        size_t port_len = 0;
        const char* p = port_ptr;
        while (*p && *p >= '0' && *p <= '9' && port_len < sizeof(port_buf) - 1) {
            port_buf[port_len++] = *p++;
        }
        port_buf[port_len] = '\0';
        
        if (port_len > 0) {
            unsigned long port_val = simple_strtoul(port_buf, NULL, 10);
            if (port_val <= 65535) {
                addr->port = htons((uint16_t)port_val);
            } else {
                return NET_ERROR_INVALID_PARAM;
            }
        }
    }
    
    // Парсим IPv6 адрес
    struct in6_addr ipv6_addr;
    if (in6_pton(addr_buf, -1, (u8*)&ipv6_addr, -1, NULL) != 1) {
        return NET_ERROR_INVALID_PARAM;
    }
    
    // Заполняем структуру
    addr->family = NET_AF_INET6;
    memcpy(addr->addr.ipv6, ipv6_addr.s6_addr, 16);
    
    return NET_SUCCESS;
}

const char* ipv4_to_string(__be32 addr, char* buffer, size_t size)
{
    u8 *bytes = (u8*)&addr;
    snprintf(buffer, size, "%u.%u.%u.%u",
             bytes[0], bytes[1], bytes[2], bytes[3]);
    return buffer;
}

const char* ipv6_to_string(const u8* addr, char* buffer, size_t size)
{
    // Простой формат: xx:xx:xx:xx:xx:xx:xx:xx
    u16 words[8];
    int i;
    
    for (i = 0; i < 8; i++) {
        words[i] = (addr[i*2] << 8) | addr[i*2 + 1];
    }
    
    // Ищем самое длинное сжатие нулей
    int best_start = -1, best_len = 0;
    int start = -1, len = 0;
    
    for (i = 0; i < 8; i++) {
        if (words[i] == 0) {
            if (start == -1) {
                start = i;
                len = 1;
            } else {
                len++;
            }
        } else {
            if (len > best_len) {
                best_start = start;
                best_len = len;
            }
            start = -1;
            len = 0;
        }
    }
    if (len > best_len) {
        best_start = start;
        best_len = len;
    }
    
    // Форматируем
    char* pos = buffer;
    size_t remaining = size;
    int written;
    
    for (i = 0; i < 8; i++) {
        if (best_len >= 2 && i == best_start) {
            // Пропускаем нули
            if (i == 0) {
                written = snprintf(pos, remaining, ":");
            } else {
                written = snprintf(pos, remaining, ":");
            }
            pos += written;
            remaining -= written;
            i += best_len - 1;
            if (i >= 7) {
                written = snprintf(pos, remaining, ":");
                pos += written;
            }
        } else {
            written = snprintf(pos, remaining, "%s%x", 
                             (i == 0) ? "" : ":", words[i]);
            pos += written;
            remaining -= written;
        }
        
        if (remaining <= 1) break;
    }
    
    return buffer;
}
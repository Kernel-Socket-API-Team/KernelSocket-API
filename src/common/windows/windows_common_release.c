#include "windows_common.h"

net_error_t windows_net_register()
{

    // Если уже инициализирован, просто возвращаем успех
    if (g_WskContext.Registered)
        return NET_SUCCESS;

    NTSTATUS Status;

    // Регистрация
    WSK_CLIENT_NPI WskClientNpi;
    WskClientNpi.ClientContext = &g_WskContext;
    WskClientNpi.Dispatch = &WskAppDispatch;

    Status = WskRegister(&WskClientNpi, &g_WskContext.Registration);
    if (!NT_SUCCESS(Status))
        return convert_status_from_windows(Status, NULL);

    g_WskContext.Registered = TRUE;

    return NET_SUCCESS;
}

net_error_t windows_net_activate(const size_t limitMS)
{

    net_error_t lib_current_status = windows_net_is_ready();
    if (lib_current_status != NET_ERROR_NOT_INITIALIZED)
        return lib_current_status;

    ULONG TimeoutMs;
    NTSTATUS Status;

    // Определяем таймаут для WskCaptureProviderNPI
    if (limitMS == 0)
    {
        // Не ждать, проверить сразу
        TimeoutMs = 0;
    }
    else if (limitMS == NET_WAIT_INFINITE)
    {
        // Ждать бесконечно
        TimeoutMs = WSK_INFINITE_WAIT;
    }
    else
    {
        // Ждать указанное количество миллисекунд
        TimeoutMs = (ULONG)limitMS;
    }

    // Захватываем провайдера (блокирует поток)
    Status = WskCaptureProviderNPI(&g_WskContext.Registration, TimeoutMs, &g_WskContext.ProviderNpi);

    if (Status == STATUS_SUCCESS)
    {
        g_WskContext.Initialized = TRUE;
        return NET_SUCCESS;
    }
    else if (Status == STATUS_TIMEOUT)
    {
        return NET_ERROR_TIMEOUT;
    }
    else
    {
        return convert_status_from_windows(Status, NULL);
    }
}

net_error_t windows_net_is_ready()
{
    if (!g_WskContext.Registered)
        return NET_ERROR_NOT_REGISTER;
    else if (!g_WskContext.Initialized)
        return NET_ERROR_NOT_INITIALIZED;
    else
        return NET_SUCCESS;
}

net_error_t windows_net_cleanup()
{
    if (g_WskContext.Registered && g_WskContext.Initialized)
    {
        WskReleaseProviderNPI(&g_WskContext.Registration);
        WskDeregister(&g_WskContext.Registration);
        g_WskContext.Initialized = FALSE;
        g_WskContext.Registered = FALSE;
    }
    else if (g_WskContext.Registered)
    {
        WskDeregister(&g_WskContext.Registration);
        g_WskContext.Registered = FALSE;
    }
    else
        return NET_ERROR_NOT_REGISTER;

    return NET_SUCCESS;
}

net_error_t windows_net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr)
{
    if (!str || !addr)
        return NET_ERROR_INVALID_PARAM;

    NTSTATUS status;
    char ip_str[256] = {0};
    char port_str[16] = {0};
    const char* colon_pos = NULL;

    if (ip_family == NET_AF_INET4)
    {
        // Ищем порт
        colon_pos = strrchr(str, ':');
        
        if (colon_pos) {
            // Копируем IP часть (до порта)
            size_t ip_len = colon_pos - str;
            if (ip_len >= sizeof(ip_str))
                return NET_ERROR_INVALID_PARAM;
            memcpy(ip_str, str, ip_len);
            ip_str[ip_len] = '\0';
            
            // Копируем порт
            size_t port_len = 0;
            const char* p = colon_pos + 1;
            while (*p && port_len < sizeof(port_str) - 1) {
                port_str[port_len++] = *p++;
            }
            port_str[port_len] = '\0';
        } else {
            // Нет порта
            size_t i = 0;
            while (str[i] && i < sizeof(ip_str) - 1) {
                ip_str[i] = str[i];
                i++;
            }
            ip_str[i] = '\0';
        }
        
        // Удаляем scope_id из IPv4 если есть
        char* percent = strchr(ip_str, '%');
        if (percent) {
            *percent = '\0';
        }
        
        // Парсим IPv4 адрес
        const char* terminator = NULL;
        IN_ADDR ip4;
        status = RtlIpv4StringToAddressA(ip_str, FALSE, &terminator, &ip4);
        
        if (NT_SUCCESS(status))
        {
            addr->family = ip_family;
            addr->addr.ipv4 = ip4.S_un.S_addr;
            addr->scope_id = 0;
            
            // Парсим порт
            if (colon_pos && port_str[0]) {
                ULONG port = 0;
                for (size_t i = 0; port_str[i]; i++) {
                    if (port_str[i] >= '0' && port_str[i] <= '9') {
                        port = port * 10 + (port_str[i] - '0');
                    } else {
                        return NET_ERROR_INVALID_PARAM;
                    }
                }
                if (port > 65535)
                    return NET_ERROR_INVALID_PARAM;
                addr->port = RtlUshortByteSwap((USHORT)port);
            }
        }
    }
    else if (ip_family == NET_AF_INET6)
    {
        IN6_ADDR ip6;
        ULONG scope_id = 0;
        USHORT port = 0;

        char normalized[256];
        status = normalize_ipv6_scope_id(str, normalized, sizeof(normalized));

        if (!NT_SUCCESS(status))
            return convert_status_from_windows(status, NULL);

        status = RtlIpv6StringToAddressExA(normalized, &ip6, &scope_id, &port);
        if (NT_SUCCESS(status))
        {
            addr->family = ip_family;
            memcpy(addr->addr.ipv6, ip6.u.Byte, 16);
            addr->scope_id = scope_id;
            if (port != 0)
                addr->port = port;  // Уже в network нотации
        }
    }
    else
    {
        return NET_ERROR_INVALID_PARAM;
    }

    return convert_status_from_windows(status, NULL);
}

net_error_t windows_net_htons(uint16_t hostshort, uint16_t* netshort)
{
    if (!netshort)
        return NET_ERROR_INVALID_PARAM;

    *netshort = RtlUshortByteSwap(hostshort);

    return NET_SUCCESS;
}

net_error_t windows_net_ntohs(uint16_t netshort, uint16_t* hostshort)
{
    if (!hostshort)
        return NET_ERROR_INVALID_PARAM;

    *hostshort = RtlUshortByteSwap(netshort);

    return NET_SUCCESS;
}

net_error_t windows_net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size,
                                          bool include_port)
{

    // Проверка валидности входных параметров
    if (!addr || !buffer || !buffer_size)
        return NET_ERROR_INVALID_PARAM;

    // Проверка, что буфер достаточного размера для адреса (+порт, если требуется)
    net_addr_str_size_t str_size = net_address_required_size(addr->family, include_port);
    if (str_size.max == 0)
        return NET_ERROR_INVALID_PARAM;
    if (buffer_size < str_size.max)
        return NET_ERROR_BUFFER_TOO_SMALL;

    // Подготовка порта
    uint16_t host_port = include_port ? RtlUshortByteSwap(addr->port) : 0;

    // Конвертация адреса в строку
    NTSTATUS status;
    ULONG size = (ULONG)buffer_size;

    if (addr->family == NET_AF_INET4)
    {
        IN_ADDR ip;
        ip.S_un.S_addr = addr->addr.ipv4;
        status = RtlIpv4AddressToStringExA(&ip, (USHORT)host_port, buffer, &size);
    }
    else if (addr->family == NET_AF_INET6)
    {
        IN6_ADDR ip;
        RtlCopyMemory(&ip, addr->addr.ipv6, 16);

        // Если scope_id = 0, то %часть не добавится
        status = RtlIpv6AddressToStringExA(&ip, addr->scope_id, (USHORT)host_port, buffer, &size);
    }
    else
    {
        return NET_ERROR_INVALID_PARAM;
    }

    return convert_status_from_windows(status, NULL);
}
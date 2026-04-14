#include "windows_common.h"

net_error_t windows_net_register () {

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
        return convert_status_from_windows(Status);

    g_WskContext.Registered = TRUE;

    return NET_SUCCESS;
}

net_error_t windows_net_activate (const size_t limitMS) {

    net_error_t lib_current_status = windows_net_is_ready();
    if (lib_current_status != NET_ERROR_NOT_INITIALIZED)
        return lib_current_status;

    ULONG TimeoutMs;
    NTSTATUS Status;

    // Определяем таймаут для WskCaptureProviderNPI
    if (limitMS == 0) {
        // Не ждать, проверить сразу
        TimeoutMs = 0;
    } else if (limitMS == NET_WAIT_INFINITE) {
        // Ждать бесконечно
        TimeoutMs = WSK_INFINITE_WAIT;
    } else {
        // Ждать указанное количество миллисекунд
        TimeoutMs = (ULONG)limitMS;
    }
    
    // Захватываем провайдера (блокирует поток)
    Status = WskCaptureProviderNPI(
        &g_WskContext.Registration,
        TimeoutMs,
        &g_WskContext.ProviderNpi
    );

    if (Status == STATUS_SUCCESS) {
        g_WskContext.Initialized = TRUE;
        return NET_SUCCESS;
    } else if (Status == STATUS_TIMEOUT) {
        return NET_ERROR_TIMEOUT;
    } else {
        return convert_status_from_windows(Status);
    }
}

net_error_t windows_net_is_ready() {
    if (!g_WskContext.Registered) return NET_ERROR_NOT_REGISTER;
    else if (!g_WskContext.Initialized) return NET_ERROR_NOT_INITIALIZED;
    else return NET_SUCCESS;
}

net_error_t windows_net_cleanup () {
    if (g_WskContext.Registered && g_WskContext.Initialized) {
        WskReleaseProviderNPI(&g_WskContext.Registration);
        WskDeregister(&g_WskContext.Registration);
        g_WskContext.Initialized = FALSE;
        g_WskContext.Registered = FALSE;
    } else if (g_WskContext.Registered) {
        WskDeregister(&g_WskContext.Registration);
        g_WskContext.Registered = FALSE;
    } else return NET_ERROR_NOT_REGISTER;

  return NET_SUCCESS;
}

net_error_t windows_net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr) {
    if (!str || !addr) return NET_ERROR_INVALID_PARAM;
    
    NTSTATUS status;
    const char* terminator = NULL;

    if (ip_family == NET_AF_INET4) {
        IN_ADDR ip4;

        status = RtlIpv4StringToAddress(str, FALSE, &terminator, &ip4);

        if (NT_SUCCESS(status)) {
            addr->family = ip_family;
            addr->addr.ipv4 = ip4.S_un.S_addr;
        }
    } else if (ip_family == NET_AF_INET6) {
        IN6_ADDR ip6;

        status = RtlIpv6StringToAddress(str, &terminator, &ip6);

        if (NT_SUCCESS(status)) {
            addr->family = ip_family;
            // Копируем 16 байт = размеру ipv6 в структуре addr
            memcpy(addr->addr.ipv6, ip6.u.Byte, 16);
        }
    } else {
        // Невалидная версия ip -> ошибка передачи параметров в функцию
        return NET_ERROR_INVALID_PARAM;
    }

    return convert_status_from_windows(status);
}

net_error_t windows_net_htons(uint16_t hostshort, uint16_t* netshort) {
    if (!netshort) 
        return NET_ERROR_INVALID_PARAM;

    *netshort = RtlUshortByteSwap(hostshort);

    return NET_SUCCESS;
}

net_error_t windows_net_ntohs(uint16_t netshort, uint16_t* hostshort) {
    if (!hostshort) 
        return NET_ERROR_INVALID_PARAM;

    *hostshort = RtlUshortByteSwap(netshort);

    return NET_SUCCESS;
}

net_error_t windows_net_address_to_string (const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port) {
    
    // Проверка валидности входных параметров
    if (!addr || !buffer || !buffer_size) return NET_ERROR_INVALID_PARAM;

    // Проверка, что буфер достаточного размера для адреса (+порт, если требуется)
    net_addr_str_size_t str_size = net_address_required_size(addr->family, include_port);
    if (str_size.max == 0) return NET_ERROR_INVALID_PARAM;
    if (buffer_size < str_size.max) return NET_ERROR_BUFFER_TOO_SMALL;

    // Подготовка порта
    uint16_t host_port = include_port ? RtlUshortByteSwap(addr->port) : 0;
    
    // Конвертация адреса в строку
    NTSTATUS status;
    ULONG size = (ULONG)buffer_size;

    if (addr->family == NET_AF_INET4) {
        IN_ADDR ip; 
        ip.S_un.S_addr = addr->addr.ipv4;
        status = RtlIpv4AddressToStringExA(&ip, (USHORT)host_port, buffer, &size);
    } else if (addr->family == NET_AF_INET6) {
        IN6_ADDR ip;
        RtlCopyMemory(&ip, addr->addr.ipv6, 16);
        // scope не поддерживается!
        status = RtlIpv6AddressToStringExA(&ip, 0, (USHORT)host_port, buffer, &size);
    } else return NET_ERROR_INVALID_PARAM;

    return convert_status_from_windows(status);
}
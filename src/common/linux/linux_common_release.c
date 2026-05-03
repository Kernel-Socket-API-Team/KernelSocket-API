#include "linux_common.h"

net_error_t linux_net_register()
{
    if (g_LinuxContext.Initialized)
        return NET_SUCCESS;
    
    g_LinuxContext.Initialized = true;
    g_LinuxContext.Activated = false;
    
    return NET_SUCCESS;
}

net_error_t linux_net_activate(const size_t limitMS)
{
    if (!g_LinuxContext.Initialized)
        return NET_ERROR_NOT_REGISTER;
    
    if (g_LinuxContext.Activated)
        return NET_SUCCESS;
    
    // В Linux инициализация мгновенна, limitMS не импользуется

    g_LinuxContext.Activated = true;
    
    return NET_SUCCESS;
}

net_error_t linux_net_is_ready()
{
    if (!g_LinuxContext.Initialized)
        return NET_ERROR_NOT_REGISTER;
    else if (!g_LinuxContext.Activated)
        return NET_ERROR_NOT_INITIALIZED;
    else
        return NET_SUCCESS;
}

net_error_t linux_net_cleanup()
{
    if (!g_LinuxContext.Initialized)
        return NET_ERROR_NOT_REGISTER;
    
    g_LinuxContext.Activated = false;
    g_LinuxContext.Initialized = false;
    
    return NET_SUCCESS;
}

net_error_t linux_net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr)
{
    if (!str || !addr) {
        return NET_ERROR_INVALID_PARAM;
    }
    
    memset(addr, 0, sizeof(net_address_t));
    
    if (ip_family == NET_AF_INET4) {
        return parse_ipv4_address(str, addr);
    } else if (ip_family == NET_AF_INET6) {
        return parse_ipv6_address(str, addr);
    }
    
    return NET_ERROR_INVALID_PARAM;
}

net_error_t linux_net_htons(uint16_t hostshort, uint16_t* netshort)
{
    *netshort = htons(hostshort);
    return NET_SUCCESS;
}

net_error_t linux_net_ntohs(uint16_t netshort, uint16_t* hostshort)
{
    *hostshort = ntohs(netshort);
    return NET_SUCCESS;
}

net_error_t linux_net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port)
{
    char ip_str[INET6_ADDRSTRLEN] = {0};
    const char* ip_result = NULL;
    int written = 0;
    
    // Проверка параметров
    if (!addr || !buffer || buffer_size == 0) {
        return NET_ERROR_INVALID_PARAM;
    }
    
    // Обнуляем буфер
    memset(buffer, 0, buffer_size);
    
    // Преобразуем IP адрес в строку
    if (addr->family == NET_AF_INET4) {
        // Для IPv4 используем простой формат
        ip_result = ipv4_to_string(addr->addr.ipv4, ip_str, sizeof(ip_str));
        
        if (!ip_result) {
            return NET_ERROR_INVALID_PARAM;
        }
        
        // Формируем строку для IPv4
        if (include_port && addr->port != 0) {
            written = snprintf(buffer, buffer_size, "%s:%d", 
                             ip_str, ntohs(addr->port));
        } else {
            written = snprintf(buffer, buffer_size, "%s", ip_str);
        }
        
        if (written < 0 || (size_t)written >= buffer_size) {
            return NET_ERROR_BUFFER_TOO_SMALL;
        }
        
    } else if (addr->family == NET_AF_INET6) {
        // Для IPv6 используем кастомный форматтер
        ip_result = ipv6_to_string(addr->addr.ipv6, ip_str, sizeof(ip_str));
        
        if (!ip_result) {
            return NET_ERROR_INVALID_PARAM;
        }
        
        // Формируем строку для IPv6
        if (include_port && addr->port != 0) {
            if (addr->scope_id != 0) {
                // Формат: "[IP%SCOPE]:PORT"
                written = snprintf(buffer, buffer_size, "[%s%%%u]:%d", 
                                 ip_str, addr->scope_id, ntohs(addr->port));
            } else {
                // Формат: "[IP]:PORT"
                written = snprintf(buffer, buffer_size, "[%s]:%d", 
                                 ip_str, ntohs(addr->port));
            }
        } else {
            if (addr->scope_id != 0) {
                // Формат: "IP%SCOPE"
                written = snprintf(buffer, buffer_size, "%s%%%u", 
                                 ip_str, addr->scope_id);
            } else {
                // Формат: "IP"
                written = snprintf(buffer, buffer_size, "%s", ip_str);
            }
        }
        
        if (written < 0 || (size_t)written >= buffer_size) {
            return NET_ERROR_BUFFER_TOO_SMALL;
        }
        
    } else {
        return NET_ERROR_INVALID_PARAM;
    }
    
    return NET_SUCCESS;
}
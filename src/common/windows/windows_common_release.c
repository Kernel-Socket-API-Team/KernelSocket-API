#include "windows_common.h"

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

// Sttubs
net_error_t windows_net_initialize () {
    return (net_error_t)0;
}

net_error_t windows_net_cleanup () {
    return (net_error_t)0;
}

net_error_t windows_net_socket_create (net_family_t s, net_protocol_t ss, int sss, net_socket_t* ssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_close (net_socket_t* s) {
    s = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_set_options (net_socket_t* s, const net_socket_options_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_get_options (net_socket_t* s, net_socket_options_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_bind (net_socket_t* s, const net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_connect (net_socket_t* s, const net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_listen (net_socket_t* s, int ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_accept (net_socket_t* s, net_address_t* ss, net_socket_t* sss) {
    s = 0;
    ss = 0;
    sss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_send (net_socket_t* s, const void* ss, size_t sss, size_t* ssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_send_to (net_socket_t* s, const void* ss, size_t sss, const net_address_t* ssss, size_t* sssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    sssss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_receive (net_socket_t* s, void* ss, size_t sss, size_t* ssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_receive_from (net_socket_t* s, void* ss, size_t sss, net_address_t* ssss, size_t* sssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    sssss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_get_local_address (net_socket_t* s, net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_get_remote_address (net_socket_t* s, net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_set_nonblocking (net_socket_t* s, int ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_can_read (net_socket_t* s, int ss, int* sss) {
    s = 0;
    ss = 0;
    sss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_can_write (net_socket_t* s, int ss, int* sss) {
    s = 0;
    ss = 0;
    sss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_socket_last_error (net_socket_t* s, const char* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t windows_net_error_string (net_error_t s, const char* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}
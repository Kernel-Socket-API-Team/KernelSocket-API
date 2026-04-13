#include "linux_common.h"

#include "windows_common.h"
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

net_error_t linux_net_register () {
    return 0;
}

net_error_t linux_net_activate (const size_t limitMS) {
    limitMS = 0;
    return 0;
}

net_error_t linux_net_is_ready() {
    return 0;
}

net_error_t linux_net_cleanup () {
    return 0;
}

net_error_t linux_net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr) {
    if (!str || !addr)
    {
        return NET_ERROR_INVALID_PARAM;
    }

    int status = 0;

    if (ip_family == NET_AF_INET4)
    {
        struct in_addr ip4;
        status = inet_pton(AF_INET, str, &ip4);

        if (status)
        {
            addr->family = ip_family;
            addr->addr.ipv4 = ip4.s_addr;
        }
    }
    else if (ip_family == NET_AF_INET6)
    {
        struct in6_addr ip6;
        status = inet_pton(AF_INET6, str, &ip6);

        if (status)
        {
            addr->family = ip_family;
            memcpy(addr->addr.ipv6, ip6.__in6_u.__u6_addr8, 16);
        }
    }

    if (!status)
    {
        return NET_ERROR_INVALID_PARAM;
    }
    else
    {
        return NET_SUCCESS;
    }
}

net_error_t linux_net_htons(uint16_t hostshort, uint16_t* netshort) {
    hostshort = 0;
    netshort = 0;
    return 0;
}

net_error_t linux_net_ntohs(uint16_t netshort, uint16_t* hostshort) {
    netshort = 0;
    hostshort = 0;
    return 0;
}

net_error_t linux_net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port) {
    if (!addr || !buffer || !buffer_size)
    {
        return NET_ERROR_INVALID_PARAM;
    }

    net_addr_str_size_t str_size = net_address_required_size(addr->family, include_port);
    if (str_size.max == 0)
    {
        return NET_ERROR_INVALID_PARAM;
    }
    else if (buffer_size < str_size.max)
    {
        return NET_ERROR_BUFFER_TOO_SMALL;
    }

    uint16_t host_port = include_port ? ntohs(addr->port) : 0;
    char port_buffer[8];
    sprintf(port_buffer, "%" PRIu16 "\n", host_port);

    int status = 0;
    uint64_t size = (uint64_t)buffer_size;

    if (addr->family == NET_AF_INET4)
    {
        struct in_addr ip4;
        ip4.s_addr = addr->addr.ipv4;
        status = inet_ntop(AF_INET, &ip4, buffer, size);
    }
    else if (addr->family == NET_AF_INET6)
    {
        struct in6_addr ip6;
        memcpy(addr->addr.ipv6, ip6.__in6_u.__u6_addr8, 16);
        status = inet_ntop(AF_INET6, &ip6, buffer, size);
    }
    strcat(buffer, port_buffer);

    if (!status)
    {
        return NET_ERROR_INVALID_PARAM;
    }
    else
    {
        return NET_SUCCESS;
    }
}

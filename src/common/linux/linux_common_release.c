#include "linux_common.h"

#include "windows_common.h"
#include <arpa/inet.h>
#include <netinet/ip.h>

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

net_error_t linux_net_address_to_string (const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port) {
    addr = 0;
    buffer = 0;
    buffer_size = 0;
    include_port = 0;
    return 0;
}

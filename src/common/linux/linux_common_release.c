#include "linux_common.h"

net_error_t linux_net_register () {
    return 0;
}

net_error_t linux_net_activate(const size_t limitMS)
{
    return 0;
}

net_error_t linux_net_is_ready()
{
    return 0;
}

net_error_t linux_net_cleanup()
{
    return 0;
}

net_error_t linux_net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr)
{
    if (!str || !addr)
    {
        return NET_ERROR_INVALID_PARAM;
    }

    int status = 0;

    struct sockaddr_storage sock_addr = {0};

    if (ip_family == NET_AF_INET4)
    {
        const char* ip4_ptr = str;
        const char* port_ptr = strchr(str, ':');

        size_t size = (size_t)(port_ptr - ip4_ptr);

        char ip4_buf[NET_IPV4_PORT_STR_MAX];
        memcpy(ip4_buf, ip4_ptr, size);
        ip4_buf[size] = '\0';

        ++port_ptr;

        status = inet_pton_with_scope(&init_net, AF_INET, ip4_buf, port_ptr, &sock_addr);

        struct sockaddr_in* ip4_addr = (struct sockaddr_in*)&sock_addr;

        if (status == 0)
        {
            addr->family = NET_AF_INET4;
            addr->port = ip4_addr->sin_port;
            addr->addr.ipv4 = ip4_addr->sin_addr.s_addr;
            addr->scope_id = 0;
        }
    }
    else if (ip_family == NET_AF_INET6)
    {
        if (*str == '[')
        {
            const char* ip6_ptr = str;
            const char* port_ptr = strchr(str, ']');

            char ip6_buf[NET_IPV6_PORT_STR_MAX];
            strncpy(ip6_buf, ip6_ptr, (size_t)(port_ptr - ip6_ptr + 1));
            port_ptr += 2;

            status = inet_pton_with_scope(&init_net, AF_INET6, ip6_buf, port_ptr, &sock_addr);
        }
        else
        {
            status = inet_pton_with_scope(&init_net, AF_INET6, str, NULL, &sock_addr);
        }

        struct sockaddr_in6* ip6_addr = (struct sockaddr_in6*)&sock_addr;

        if (status)
        {
            addr->family = NET_AF_INET6;
            addr->port = ip6_addr->sin6_port;
            memcpy(addr->addr.ipv6, ip6_addr->sin6_addr.in6_u.u6_addr8, 16);
            addr->scope_id = ip6_addr->sin6_scope_id;
        }
    }

    if (status != 0)
    {
        return NET_ERROR_INVALID_PARAM;
    }
    else
    {
        return NET_SUCCESS;
    }
}


net_error_t linux_net_htons(uint16_t hostshort, uint16_t* netshort)
{
    hostshort = 0;
    netshort = 0;
    return 0;
}

net_error_t linux_net_ntohs(uint16_t netshort, uint16_t* hostshort)
{
    netshort = 0;
    hostshort = 0;
    return 0;
}

net_error_t linux_net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port) {
    // if (!addr || !buffer || !buffer_size)
    // {
    //     return NET_ERROR_INVALID_PARAM;
    // }

    // net_addr_str_size_t str_size = net_address_required_size(addr->family, include_port);
    // if (str_size.max == 0)
    // {
    //     return NET_ERROR_INVALID_PARAM;
    // }
    // else if (buffer_size < str_size.max)
    // {
    //     return NET_ERROR_BUFFER_TOO_SMALL;
    // }

    // uint16_t host_port = include_port ? ntohs(addr->port) : 0;
    // char port_buffer[8];
    // sprintf(port_buffer, "%" PRIu16 "\n", host_port);

    // int status = 0;
    // uint64_t size = (uint64_t)buffer_size;

    // if (addr->family == NET_AF_INET4)
    // {
    //     struct in_addr ip4;
    //     ip4.s_addr = addr->addr.ipv4;
    //     status = inet_ntop(AF_INET, &ip4, buffer, size);
    // }
    // else if (addr->family == NET_AF_INET6)
    // {
    //     struct in6_addr ip6;
    //     memcpy(addr->addr.ipv6, ip6.in6_u.u6_addr8, 16);
    //     status = inet_ntop(AF_INET6, &ip6, buffer, size);
    // }
    // strcat(buffer, port_buffer);

    // if (!status)
    // {
    //     return NET_ERROR_INVALID_PARAM;
    // }
    // else
    // {
    //     return NET_SUCCESS;
    // }
    return 0;
}

#include "common.h"

net_addr_str_size_t net_address_required_size(int family, bool include_port)
{
    switch (family)
    {
    case NET_AF_INET4:
        return (net_addr_str_size_t){.min = include_port ? NET_IPV4_PORT_STR_MIN : NET_IPV4_STR_MIN,
                                     .max = include_port ? NET_IPV4_PORT_STR_MAX : NET_IPV4_STR_MAX};

    case NET_AF_INET6:
        return (net_addr_str_size_t){.min = include_port ? NET_IPV6_PORT_STR_MIN : NET_IPV6_STR_MIN,
                                     .max = include_port ? NET_IPV6_PORT_STR_MAX : NET_IPV6_STR_MAX};

    default:
        return (net_addr_str_size_t){0, 0};
    }
}
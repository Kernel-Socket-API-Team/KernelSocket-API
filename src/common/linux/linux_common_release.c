#include "linux_common.h"

net_error_t linux_net_register()
{
    if (g_LinuxContext.Initialized)
        return NET_SUCCESS;
    
    g_LinuxContext.Initialized = TRUE;
    g_LinuxContext.Activated = FALSE;
    
    return NET_SUCCESS;
}

net_error_t linux_net_activate(const size_t limitMS)
{
    if (!g_LinuxContext.Initialized)
        return NET_ERROR_NOT_REGISTER;
    
    if (g_LinuxContext.Activated)
        return NET_SUCCESS;
    
    // В Linux инициализация мгновенна, limitMS не импользуется

    g_LinuxContext.Activated = TRUE;
    
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
    
    g_LinuxContext.Activated = FALSE;
    g_LinuxContext.Initialized = FALSE;
    
    return NET_SUCCESS;
}

net_error_t linux_net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr)
{
    ip_family = 0;
    addr = 0;
    return 0;
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

net_error_t linux_net_address_to_string(const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port)
{
    buffer = 0;
    buffer_size = 0;
    include_port = 0;
    return 0;
}
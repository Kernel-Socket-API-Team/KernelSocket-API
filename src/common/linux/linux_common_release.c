#include "linux_common.h"

net_error_t linux_net_register ();

net_error_t linux_net_activate (const size_t limitMS);

net_error_t linux_net_is_ready();

net_error_t linux_net_cleanup();

net_error_t linux_net_address_parse(const char* str, net_family_t ip_family, net_address_t* addr);

net_error_t linux_net_htons(uint16_t hostshort, uint16_t* netshort);

net_error_t linux_net_ntohs(uint16_t netshort, uint16_t* hostshort);

net_error_t linux_net_address_to_string (const net_address_t* addr, char* buffer, size_t buffer_size, bool include_port);

net_error_t linux_net_socket_last_error(net_socket_t *sock, net_error_t error);

net_error_t linux_net_socket_last_platform_error(net_socket_t* sock, const void* platform_error);
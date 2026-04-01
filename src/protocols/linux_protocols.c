#include "../../adapters/linux/linux_adapter.h"

net_error_t linux_net_socket_create(net_family_t family, net_protocol_t protocol, int flags, net_socket_t* socket_out);

net_error_t linux_net_socket_close(net_socket_t* sock);

net_error_t linux_net_socket_set_options(net_socket_t* sock, const net_socket_options_t* opts);

net_error_t linux_net_socket_get_options(net_socket_t* sock, net_socket_options_t* opts);

net_error_t linux_net_socket_bind(net_socket_t* sock, const net_address_t* addr);

net_error_t linux_net_socket_connect(net_socket_t* sock, const net_address_t* addr);

net_error_t linux_net_socket_send(net_socket_t* sock, const void* data, size_t size, size_t* sent);

net_error_t linux_net_socket_receive(net_socket_t* sock, void* buffer, size_t buffer_size, size_t* received);

net_error_t linux_net_socket_get_local_address(net_socket_t* sock, net_address_t* addr);

net_error_t linux_net_socket_get_remote_address(net_socket_t* sock, net_address_t* addr);

net_error_t linux_net_socket_set_nonblocking(net_socket_t* sock, int enable);

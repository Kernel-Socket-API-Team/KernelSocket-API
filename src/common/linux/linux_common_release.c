#include "linux_common.h"

#include <linux/module.h>
// ХАК: MODULE_LICENSE здесь нужен только для modpost
// Фактическая лицензия определяется в конечном драйвере
MODULE_LICENSE("GPL");
#include <linux/printk.h> 

// Sttubs
net_error_t linux_net_initialize () {
    panic("linux_net_initialize is work!\n");
    return (net_error_t)0;
}

net_error_t linux_net_cleanup () {
    return (net_error_t)0;
}

net_error_t linux_net_socket_create (net_family_t s, net_protocol_t ss, int sss, net_socket_t* ssss) {
    s = (net_family_t)0;
    ss = (net_protocol_t)0;
    sss = 0;
    ssss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_close (net_socket_t* s) {
    s = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_set_options (net_socket_t* s, const net_socket_options_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_get_options (net_socket_t* s, net_socket_options_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_bind (net_socket_t* s, const net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_connect (net_socket_t* s, const net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_listen (net_socket_t* s, int ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_accept (net_socket_t* s, net_address_t* ss, net_socket_t* sss) {
    s = 0;
    ss = 0;
    sss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_send (net_socket_t* s, const void* ss, size_t sss, size_t* ssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_send_to (net_socket_t* s, const void* ss, size_t sss, const net_address_t* ssss, size_t* sssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    sssss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_receive (net_socket_t* s, void* ss, size_t sss, size_t* ssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_receive_from (net_socket_t* s, void* ss, size_t sss, net_address_t* ssss, size_t* sssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    sssss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_address_parse(const char* s, net_family_t ss, net_address_t* sss) {
    s = 0;
    ss = (net_family_t)0;
    sss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_address_to_string (const net_address_t* s, char* ss, size_t sss, bool ssss) {
    s = 0;
    ss = 0;
    sss = 0;
    ssss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_get_local_address (net_socket_t* s, net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_get_remote_address (net_socket_t* s, net_address_t* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_set_nonblocking (net_socket_t* s, int ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_can_read (net_socket_t* s, int ss, int* sss) {
    s = 0;
    ss = 0;
    sss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_can_write (net_socket_t* s, int ss, int* sss) {
    s = 0;
    ss = 0;
    sss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_socket_last_error (net_socket_t* s, const char* ss) {
    s = 0;
    ss = 0;
    return (net_error_t)0;
}

net_error_t linux_net_error_string (net_error_t s, const char* ss) {
    s = (net_error_t)0;
    ss = 0;
    return (net_error_t)0;
}
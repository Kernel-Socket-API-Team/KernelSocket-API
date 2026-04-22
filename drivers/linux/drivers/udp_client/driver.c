#include <linux/in.h>
#include <linux/inet.h> // для in4_pton
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/net.h>
#include <net/sock.h>

static struct socket* sock = NULL;
static struct sockaddr_in dest_addr;
static struct sockaddr_in src_addr;

static int send_udp_kernel(char* msg, int len)
{
    struct msghdr msgHdr = {0};
    struct kvec kv;
    int status;
    u32 ip_addr;

    // Создание сокета
    status = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (status < 0)
    {
        printk(KERN_ERR "sock_create_kern failed: %d\n", status);
        return status;
    }
    printk(KERN_INFO "Kernel socket created successfully\n");

    // Бинд к нужному интерфейсу с использованием
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sin_family = AF_INET;

    // Конвертация source IP
    if (!in4_pton("0.0.0.0", -1, (u8*)&ip_addr, -1, NULL))
    {
        printk(KERN_ERR "Failed to convert source IP address\n");
        sock_release(sock);

        // Возвращаем ошибку
        return -EINVAL;
    }
    src_addr.sin_addr.s_addr = ip_addr;
    src_addr.sin_port = 0; // Порт 0 = ядро выберет случайный свободный порт

    status = kernel_bind(sock, (struct sockaddr*)&src_addr, sizeof(src_addr));
    if (status < 0)
    {
        printk(KERN_ERR "kernel_bind failed: %d\n", status);
        sock_release(sock);
        return status;
    }

    // Выведем информацию о том, на какой интерфейс мы привязались
    printk(KERN_INFO "Socket bound to %pI4\n", &src_addr.sin_addr);

    // Настройка адреса получателя
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9003);

    if (!in4_pton("192.168.0.112", -1, (u8*)&dest_addr.sin_addr.s_addr, -1, NULL))
    {
        printk(KERN_ERR "Failed to convert destination IP address\n");
        sock_release(sock);
        return -EINVAL;
    }

    // Подготовка сообщения
    kv.iov_base = (void*)msg;
    kv.iov_len = len;

    msgHdr.msg_name = &dest_addr;
    msgHdr.msg_namelen = sizeof(dest_addr);

    // Отправка
    printk(KERN_INFO "Sending to %pI4:%d\n", &dest_addr.sin_addr, ntohs(dest_addr.sin_port));
    status = kernel_sendmsg(sock, &msgHdr, &kv, 1, len);

    if (status < 0)
    {
        printk(KERN_ERR "kernel_sendmsg failed: %d\n", status);
    }
    else
    {
        printk(KERN_INFO "Sent %d bytes via kernel_sendmsg: %s\n", status, msg);
    }

    return status;
}

static int __init my_udp_module_init(void)
{
    char message[] = "Hello world!";
    printk(KERN_INFO "Initializing kernel UDP sender module with explicit bind\n");
    send_udp_kernel(message, strlen(message));
    return 0;
}

static void __exit my_udp_module_exit(void)
{
    if (sock)
    {
        sock_release(sock);
        printk(KERN_INFO "Kernel socket released\n");
    }
    printk(KERN_INFO "Kernel UDP sender module exited\n");
}

module_init(my_udp_module_init);
module_exit(my_udp_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel module sending UDP via kernel_sendmsg with explicit bind");
MODULE_AUTHOR("Igor");
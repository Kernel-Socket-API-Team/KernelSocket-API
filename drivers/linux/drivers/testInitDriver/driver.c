#include "../../../../include/ksockapi.h"

MODULE_LICENSE("GPL");

static struct task_struct *g_worker_thread = NULL;

static int network_worker_thread(void *data)
{
    net_error_t err;
    
    err = net_is_ready();
    if (err != NET_SUCCESS)
        err = net_activate(NET_WAIT_INFINITE);
    
    if (err == NET_SUCCESS) {
        // Работа с сокетами
        printk(KERN_INFO "[TEST] Library ready for socket operations\n");
    } else if (err == NET_ERROR_TIMEOUT) {
        printk(KERN_ERR "[TEST] Activation timeout\n");
    } else {
        printk(KERN_ERR "[TEST] Activation failed: %d\n", err);
    }
    
    return 0;
}

static int __init test_init(void)
{
    net_error_t err;
    
    err = net_register();
    if (err != NET_SUCCESS)
        return -1;
    
    g_worker_thread = kthread_run(network_worker_thread, NULL, "net_worker");
    if (IS_ERR(g_worker_thread)) {
        net_cleanup();
        return PTR_ERR(g_worker_thread);
    }
    
    return 0;
}

static void __exit test_exit(void)
{
    if (g_worker_thread) {
        kthread_stop(g_worker_thread);
        g_worker_thread = NULL;
    }
    
    net_cleanup();
}

module_init(test_init);
module_exit(test_exit);
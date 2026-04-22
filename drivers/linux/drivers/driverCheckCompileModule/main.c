#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../../../../include/ksockapi.h"
#include "../../../../src/adapters/linux/linux_adapter.h"

static int __init test_init(void)
{
    printk(KERN_INFO "Test driver loaded\n");
    linux_net_initialize();
    return 0;
}

static void __exit test_exit(void)
{
    printk(KERN_INFO "Test driver unloaded\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Test");
MODULE_DESCRIPTION("Test driver");
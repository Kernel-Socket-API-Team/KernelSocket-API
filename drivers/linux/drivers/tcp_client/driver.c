#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static void __exit minimal_driver_exit(void) {
    printk(KERN_INFO "Minimal Driver: Unloaded\n");
}

static int __init minimal_driver_init(void) {
    printk(KERN_INFO "Minimal Driver: Loaded!\n");
    return 0; 
}

// Регистрируем функции загрузки и выгрузки
module_init(minimal_driver_init);
module_exit(minimal_driver_exit);

// Обязательная информация о модуле
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Minimal Linux Kernel Module");
MODULE_VERSION("1.0");
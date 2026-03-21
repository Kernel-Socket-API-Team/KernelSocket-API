# Использование и сборка библиотеки под Linux

## Для кого этот документ

Данная инструкция предназначена для разработчиков, которые хотят собрать библиотеку из исходного кода или подключить её к своему модулю ядра.  
Рядовым пользователям достаточно готового модуля (`.ko`) и заголовочного файла из папки `bin`.

---

## Требования

| Компонент | Назначение |
|-----------|------------|
| Ядро Linux| Целевая платформа |
| linux-headers-$(uname -r) | Заголовочные файлы ядра |
| make | Система сборки |

---

## Сборка библиотеки

В папке `compileLibraryForLinux` находится файл `ksockapi_sources.c`, который объединяет все необходимые исходники библиотеки.   Этот файл подключается в пользовательском Makefile для включения всех компонентов библиотеки в сборку.

---

## Подключение библиотеки к вашему проекту

### 1. Заголовочные файлы

Добавьте путь к папке `include/` в `ccflags-y`:
```makefile
ccflags-y += -I KernelSocket-API/include
```

### 2. Пути к внутренним заголовкам
Для корректной компиляции исходников библиотеки необходимо добавить путь к папке `src`:
```makefile
ccflags-y += -I KernelSocket-API/src
```

### 3. Исходные файлы библиотеки
Подключите файл `ksockapi_sources.c` через список объектных файлов вашего модуля:
```makefile
my_driver-y += KernelSocket-API/compileLibraryForLinux/ksockapi_sources.o
```
### Полный пример пользовательского Makefile
```makefile
KERNEL_VERSION := 6.8.0-101-generic
KDIR := /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)

obj-m += my_driver.o

my_driver-y := \
    main.o \
    KernelSocket-API/compileLibraryForLinux/ksockapi_sources.o

ccflags-y += -I KernelSocket-API/include
ccflags-y += -I KernelSocket-API/compileLibraryForLinux/src

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

### Пример использования библиотеки в коде
```c
#include <ksockapi.h>

// ...
```

## Как это работает

### Особенности сборки в Linux (KBUILD)

Система сборки KBUILD требует, чтобы **все исходные файлы модуля компилировались в рамках одной сборочной единицы**. Подключение внешних объектных файлов может привести к ошибкам линковки.

Поэтому все исходники библиотеки объединены в файл `ksockapi_sources.c`, который подключается к вашему модулю и компилируется вместе с ним.

### Платформозависимая компиляция

Диспетчер на этапе компиляции определяет целевую платформу с помощью макросов:

```c
// src/dispatcher/dispatcher.c

const net_vtable_dispatcher* vtable =
#ifdef _WIN32
    &windows_vtable;
#else
    &linux_vtable;
#endif
```
Windows-код при этом плоностью исключается из сборки под Linux, что упрощает зависимости и ускоряет компиляцию.

---

## Связанные разделы
- [API Reference](api/) — описание функций, структур и кодов ошибок
- [Архитектура проекта](../architecture.md) — общее устройство и концепция API
- [Использование под Windows](windows-build.md) — инструкция по использованию API под Windows
# KernelSocket API

> Универсальный C API для работы с TCP/UDP на уровне ядра Windows и Linux

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)]()

---

## О проекте

KernelSocket предоставляет единый интерфейс для сетевых операций в режиме ядра, абстрагируя различия между WDK (Windows) и kernel sockets (Linux). Позволяет писать кроссплатформенные драйверы и модули ядра, работающие с TCP/UDP без дублирования кода.

---

## Документация

Вся информация об архитектуре, сборке и использовании API — в разделе **[docs](docs/README.md)**.

---

## Быстрый старт

### Windows
1. Подключите заголовочный файл:
   ```c
   #include <kernelsocket.h>
   ```

2. Дополнительные каталоги включаемых файлов: KernelSocket-API/include

3. Добавьте KernelSocket-API/compileLibraryForWindows/bin/kernelsocket.lib в  дополнительные зависимости компоновщика

Подробная инструкция: [Использование и сборка библиотеки под Windows](docs/build-docs/windows-build.md).

### Linux

1. Подключите заголовочный файл:
   ```c
   #include <kernelsocket.h>
   ```
2. В Makefile добавьте пути к заголовкам и исходникам библиотеки:
    ```makefile
    ccflags-y += -I KernelSocket-API/include
    ccflags-y += -I KernelSocket-API/compileLibraryForLinux/src

    my_driver-y += KernelSocket-API/compileLibraryForLinux/ksockapi_sources.o
    ```
Подробная инструкция: [Использование и сборка библиотеки под Linux](docs/build-docs/linux-build.md).
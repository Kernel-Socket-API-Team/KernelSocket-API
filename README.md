# KernelSocket API

**Универсальный API для работы с TCP/UDP на уровне ядра Windows и Linux**

## О проекте

KernelSocket предоставляет единый интерфейс для сетевых операций в режиме ядра. API реализует базовые протоколы TCP/UDP через системные вызовы ОС, позволяя писать драйверы, не зависящие от платформы.

### Платформы
- **Windows**: WSK (Winsock Kernel), KMDF, WDK
- **Linux**: kernel sockets, netfilter, sk_buff

## Технологии

- **Язык**: C (C++ только для инфраструктуры, без STL/RTTI)
- **Windows**: WDK, Windows SDK
- **Linux**: kernel headers, GCC/Clang, Kbuild

## Отладка

- **Хост**: Windows + Visual Studio + WinDbg
- **Таргет**: виртуальная машина (VMware/VirtualBox) с включенным Test Signing
- **Сеть**: две ВМ для тестирования клиент-серверного взаимодействия

## Статус

В разработке

---

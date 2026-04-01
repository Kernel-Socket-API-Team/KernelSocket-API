# Инициализация Kernel Socket API

Данный раздел описывает поведение функций инициализации библиотеки. Эти функции обеспечивают корректную подготовку внутренней инфраструктуры API для дальнейшей работы с сокетами и являются обязательными к использованию перед вызовом любых других функций.

## Обозор функций
* `net_error_t net_register(void)`;
* `net_error_t net_activate(const size_t limitMS)`;
* `net_error_t net_is_ready(void)`;
* `net_error_t net_cleanup(void)`;

## Общая схема инициализации

Перед использованием API необходимо выполнить следующий порядок действий:

1. Вызвать `net_register()` — запуск инициализации (неблокирующий).
2. Вызвать `net_activate()` — ожидание завершения инициализации (может блокировать поток).
3. Проверить состояние через `net_is_ready()` (опционально).

## `net_register`

```c
net_error_t net_register(void);
```

### Описание

Функция запускает процесс инициализации библиотеки.

### Поведение

* **НЕ блокирует вызывающий поток**.
* В случае повторного вызова, новая регистрация производится только в случае если прошлый вызов вернул ошибку.
* Не гарантирует, что библиотека готова к использованию сразу после возврата.

### Возвращаемые значения

* `NET_SUCCESS` — инициализация успешно запущена
* [Другие ошибки связанные с работой библиотек](./errors.md)


## `net_activate`

```c
net_error_t net_activate(const size_t limitMS);
```

### Описание

Функция ожидает завершения инициализации библиотеки.

### Поведение

* **МОЖЕТ блокировать поток**.
* Рекомендуется вызывать **в отдельном рабочем потоке**, чтобы не блокировать основной поток выполнения.
* Поведение зависит от параметра `limitMS`:

  * `-1` — ожидание без ограничения по времени (аналог "бесконечного ожидания")
  * `0` — ожидание не производится
  * `> 0` — ожидание с таймаутом в миллисекундах

### Особенности

* В случае успешного выполнения библиотека гарантированно готова к работе.

### Возвращаемые значения

* `NET_SUCCESS` — инициализация завершена успешно
* `NET_ERROR_NOT_REGISTER` — `net_register` не был вызван
* `NET_ERROR_TIMEOUT` — превышено время ожидания активации
* [Другие ошибки связанные с работой библиотек](./errors.md)

## `net_is_ready`

```c
net_error_t net_is_ready(void);
```

### Описание

Проверяет, завершена ли инициализация библиотеки.

### Поведение

* **НЕ блокирует поток**.
* Выполняет мгновенную проверку состояния.
* Не инициирует и не ожидает инициализацию.

### Возвращаемые значения

* `NET_SUCCESS` — библиотека готова к использованию
* `NET_ERROR_NOT_INITIALIZED` — инициализация не завершена или не была начата
* `NET_ERROR_NOT_REGISTER` — вызов библиотеки не был зарегистрирован

---

## `net_cleanup`

```c
net_error_t net_cleanup(void);
```

### Описание

Освобождает ресурсы, выделенные библиотекой. При этом, важно вызывать функцию даже если не производилась активация (например, в случае ошибки), но регистрация была успешно выполнена.

### Поведение

* Должна вызываться при завершении работы.
* Закрывает внутренние дескрипторы, освобождает память и завершает фоновые задачи.
* После вызова библиотека считается неинициализированной.

### Ограничения

* Не рекомендуется вызывать:

  * пока выполняются сетевые операции
  * пока активны сокеты

### Возвращаемые значения

* `NET_SUCCESS` — успешное завершение
* `NET_ERROR_NOT_REGISTER` — библиотека не была инициализирована

## Важные замечания

* `net_register` и `net_activate` логически разделяются:

  * **запуск регистрации**
  * **запуск активации**
* Это позволяет:
  * избежать блокировки критических потоков
  * гибко управлять инициализацией в kernel-среде
* Поведение одинаково для:

  * Windows Kernel
  * Linux Kernel


## Примеры использования

### Windows

```c
#include <ksockapi.h>

// HANDLE потока
static PETHREAD g_WorkerThread = NULL;

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  if (g_WorkerThread) {
    KeWaitForSingleObject(g_WorkerThread, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(g_WorkerThread);
    g_WorkerThread = NULL;
  }

  net_cleanup();
}

VOID NetworkWorkerThread(PVOID Context) {
  UNREFERENCED_PARAMETER(Context);

  net_error_t err = net_is_ready();

  if (err != NET_SUCCESS) 
      err = net_activate(NET_WAIT_INFINITE); // 15 секунд
 
  if (err == NET_SUCCESS) {
    // Работа с сокетами
    // ...
  } else if (err == NET_ERROR_TIMEOUT) {
    // обработка таймаута
    // ...
  }

  PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  NTSTATUS Status;
  net_error_t err;

  err = net_register();
  if (err != NET_SUCCESS) {
    return STATUS_UNSUCCESSFUL;
  }

  HANDLE hThread;

  Status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                NetworkWorkerThread, NULL);

  if (!NT_SUCCESS(Status)) {
    net_cleanup();
    return Status;
  }

  Status = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType,
                                KernelMode, (PVOID *)&g_WorkerThread, NULL);

  ZwClose(hThread);

  if (!NT_SUCCESS(Status)) {
    net_cleanup();
    return Status;
  }

  return STATUS_SUCCESS;
}
```

### Linux

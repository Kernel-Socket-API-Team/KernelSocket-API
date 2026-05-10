# Тестирование

Тестирование KernelSocket API проводилось с целью проверки корректности работы сетевых операций (TCP/UDP) в режиме ядра при взаимодействии между различными операционными системами (Windows 10, Windows 11, Ubuntu 22.04) как в однородных, так и в перекрёстных конфигурациях. Особое внимание уделялось проверке совместимости при передаче данных между приложениями пользовательского режима и драйверами, а также прямым соединениям «ядро-ядро». Дополнительной задачей являлось тестирование поддержки IPv6 (включая link-local адреса с scope ID), что представляет собой нетривиальную задачу в среде с виртуальными машинами.

Все тесты были разделены на три основные группы согласно методике, описанной ниже. В данной главе представлены конфигурация тестового окружения, используемые методики, описание эталонных user-mode приложений и сводные результаты тестирования для всех перекрёстных комбинаций «клиент-сервер».

## Окружение тестирования
Тестирование проводилось в гетерогенной среде, сочетающей физический хост и две виртуальные машины (ВМ). Все вычислительные узлы были объединены в изолированную сеть через виртуальный коммутатор гипервизора с типом host-only (vmnet1). Это гарантирует, что тестовый трафик не выходит за пределы тестового стенда, а адресация остается детерминированной.

**Физический хост**

- **ОС**: Windows 10 Pro
- **Роль**: запуск user‑mode приложений для Windows (как клиента, так и сервера).
- **Сетевой интерфейс**: vmnet1 (статический IP, назначается вручную).

**Виртуальные машины (гипервизор VMware Workstation)**

- Две ВМ с ОС Ubuntu 22.04 и Windows 10 (с включенным режимом Test Signing для загрузки тестовых драйверов).
- Каждая ВМ подключена к тому же виртуальному коммутатору vmnet1 со статическим IP.
- Роли ВМ:
    - Запуск user‑mode приложений для Linux (клиент/сервер).
    - Загрузка и тестирование драйверов.

**Настройки брандмауэра**: на обоих Windows-узлах брандмауэр отключен для общедоступного профиля сети (Public). Это необходимо для исключения блокировки тестового трафика, проходящего через интерфейс vmnet1, который операционная система Windows классифицирует как общедоступную сеть.

Схема адресации (пример):
| Узел | IPv4 (vmnet1) | IPv6 Link-Local (vmnet1) | Scope ID | Назначение |
|------|---------------|--------------------------|----------|-------------|
| Хост (Windows 10) | 192.168.68.1 | fe80::4e3a:8d0c:b712:d139 | 6 | Windows user‑mode клиент/сервер |
| ВМ Windows 11 | 192.168.68.128 | fe80::a921:2d1f:61fb:8942 | 9 | Windows-драйверы |
| ВМ Ubuntu 22.04 | 192.168.68.129 | fe80::70f:68b4:f07f:e3ef | 2(ens33) | Linux user‑mode / Linux-драйвер |
Все узлы доступны друг для друга по IP; маршрутизация между ними осуществляется виртуальным коммутатором vmnet1 без выхода на физическую сеть.

## Методики тестирования
Цель тестирования — проверить работоспособность и совместимость KernelSocket API при передаче данных между компонентами, работающими в user mode и kernel mode, на различных операционных системах. Тестирование построено по перекрёстной схеме с чередованием ролей клиента и сервера для всех возможных сочетаний узлов и режимов.

Тестирование разбито на три основные группы сценариев, в каждой из которых фиксированная пара «тип клиента — тип сервера». Внутри каждой группы роли клиент и сервер меняются местами (т.е. если в первом запуске узел A был клиентом, а B — сервером, то во втором запуске A становится сервером, B — клиентом).

### Группа 1: User mode (Windows 10 хост) ↔ Kernel mode (Windows 11 ВМ)
В этой группе проверяется взаимодействие приложений на хосте Windows 10 с драйверами на ВМ Windows 11:

| Роль клиента | Роль сервера | Протоколы | Что проверяется |
|--------------|--------------|-----------|------------------|
| Windows 10 (user mode) | Windows 11 (драйвер) | TCP, UDP | Передача данных от приложения к драйверу |
| Windows 11 (драйвер) | Windows 10 (user mode) | TCP, UDP | Передача данных от драйвера к приложению |

**Пример теста:**

- Приложение на Windows 10 отправляет строку "Hello from Win10 user" драйверу на Windows 11. Драйвер возвращает подтверждение.

- Затем драйвер выступает в роли клиента и отправляет данные приложению-серверу на Windows 10.

### Группа 2: User mode (Ubuntu ВМ) ↔ Kernel mode (Windows 11 ВМ)
В этой группе проверяется кросс-платформенное взаимодействие между Linux-приложениями (user mode на Ubuntu) и Windows-драйверами (kernel mode на Windows 11):

| Роль клиента | Роль сервера | Протоколы | Что проверяется |
|--------------|--------------|-----------|------------------|
| Ubuntu 22.04 (user mode) | Windows 11 (драйвер) | TCP, UDP | Linux-приложение → Windows-драйвер |
| Windows 11 (драйвер) | Ubuntu 22.04 (user mode) | TCP, UDP | Windows-драйвер → Linux-приложение |

**Особенность:** в этой группе проверяется корректность преобразования сетевых порядков байт и совместимость стеков TCP/UDP между Linux и Windows на уровне «приложение — драйвер».

### Группа 3: Kernel mode (Ubuntu ВМ) ↔ Kernel mode (Windows 11 ВМ)
В этой группе оба узла работают в режиме ядра: на Ubuntu загружен Linux-драйвер, на Windows 11 — Windows-драйвер. Это наиболее сложный сценарий, проверяющий прямую передачу данных между драйверами разных ОС.

| Роль клиента | Роль сервера | Протоколы | Что проверяется |
|--------------|--------------|-----------|------------------|
| Ubuntu 22.04 (драйвер) | Windows 11 (драйвер) | TCP, UDP | Linux-драйвер → Windows-драйвер |
| Windows 11 (драйвер) | Ubuntu 22.04 (драйвер) | TCP, UDP | Windows-драйвер → Linux-драйвер |

**Особенность:** тестирование проводится без участия user-mode приложений. Это позволяет оценить «чистую» производительность межплатформенного взаимодействия на уровне ядра.

## Реализация тестовых программ для user-mode режима

Для тестирования взаимодействия между компонентами в различных конфигурациях были разработаны четыре эталонные программы (клиент и сервер для Linux и Windows), работающие в пользовательском режиме. Эти программы не используют KernelSocket API, а реализованы на стандартных сокетных интерфейсах соответствующих операционных систем.

### Программы для Linux (Ubuntu 22.04)

**Файлы:**
- `tests/testApps/echo_client_linux.c`
- `tests/testApps/echo_server_linux.c`

Используемые технологии и API:

|   Платформа    | Сокетный API |  Компилятор  |
|----------------|--------------|--------------|
| Linux / Ubuntu 22.04 | POSIX Socket API | GCC |

**Ключевые особенности:**

- Поддержка IPv4 и IPv6 (включая link-local адреса с scope ID)
- TCP и UDP протоколы
- Эхо-сервер с последовательной обработкой клиентов

**Компиляция:**
```bash
gcc echo_client_linux.c -o ./build/echo_client_linux
gcc echo_server_linux.c -o ./build/echo_server_linux
```

### Программы для Windows (Windows 10 / Windows 11)

**Файлы:**
- `tests/testApps/echo_client_windows.c`
- `tests/testApps/echo_server_windows.c`

Используемые технологии и API:

|   Платформа    | Сокетный API |  Компилятор  |
|----------------|--------------|--------------|
| Windows 10/11 | Winsock2 API | MSVC (cl.exe) |

**Ключевые особенности:**

- Обязательная инициализация Winsock через WSAStartup()
- Поддержка IPv4 и IPv6
- TCP и UDP протоколы

Компиляция (Developer PowerShell for VS):
```bash
cl echo_client_windows.c ws2_32.lib /Fe:build\echo_client_windows.exe
cl echo_server_windows.c ws2_32.lib /Fe:build\echo_server_windows.exe
```

### Обоснование выбора нативных API
Тестовые программы реализованы на нативных сокетных API каждой ОС, а не через KernelSocket, поскольку:

* POSIX Socket API (Linux) — стандартный интерфейс пользовательского пространства для работы с сетью в Unix-подобных системах.

* Winsock2 API (Windows) — стандартный сокетный интерфейс Windows, обязательный для любых сетевых приложений.

Это позволяет проверять драйверы KernelSocket на совместимость с эталонным поведением операционной системы, исключая влияние тестируемой библиотеки на замеры.

## Результаты тестирования (User mode - Kernel mode)

| № | Клиент | Сервер | Протокол | IP | Эхо получен | Вывод дебагера | Вывод приложения |
|---|--------|--------|----------|----|-------------|------------------------|--|
| 1 | Windows 10 (user) | Windows 11 (driver) | TCP | IPv4 | ДА | `[ECHO] Driver loaded (TCP:4444, UDP:4445)`<br/>`[TCP] Server ready on port 4444`<br/>`[TCP] Client connected`<br/>`[TCP] [192.168.68.1:42484] >> Hello from user mode!`<br/>`[TCP] Client disconnected` | `[Client] TCP IPv4 connecting to 192.168.68 128:4444`<br>`Connected! Enter messages (type 'quit' to exit)`<br/>`> Hello from user mode!`<br/>`Sent: Hello from user mode!`<br/>`Echoed: Hello from user mode!` |
| 2 | Windows 10 (user) | Windows 11 (driver) | UDP | IPv4 | ДА | `[ECHO] Driver loaded (TCP:4444, UDP:4445)`<br/>`[UDP] Server ready on port 4445`<br/>`[UDP] [192.168.68.1:14059] >> Hello from user mode!(udp)`| `[Client] UDP IPv4 connecting to 192.168.68.128:4445`<br/>`UDP mode. Enter messages (type 'quit' to exit)`<br/>`> Hello from user mode(udp)`<br/>`Sent: Hello from user mode!(udp)`<br/>`Echoed: Hello from user mode!(udp)` |
| 3 | Windows 11 (driver) | Windows 10 (user) | TCP | IPv4 | ДА | `[Client] Driver loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[TCP Client] Connecting to 192.168.68.1:4444`<br/>`[TCP Client] Connected to 192.168.68.1:4444`<br/>`[TCP Client] Sent 22 bytes: Hello from TCP client!`<br/>`[TCP Client] Received echo: Hello from TCP client!` | `[Server] TCP IPv4 on port 4444`<br/>`Listening for connections...`<br/>`[Client] IPv4: 192.168.68.128:49677`<br/>`Received: Hello from TCP client!`<br/>`Echoed: Hello from TCP client!`<br/>`[Client] Disconnected`
| 4 | Windows 11 (driver) | Windows 10 (user) | UDP | IPv6 | ДА | `[Client] Driver loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[UDP Client] Sending to fe80::4e3a:8d0c:b712:d139%9:4445: Hello from UDP client!`<br/>`[UDP Client] Sent 22 bytes`<br/>`[UDP Client] Received echo: Hello from UDP client!` | `[Server] UDP IPv6 on port 4445`<br/>`Waiting for datagrams...`<br/>`[Client] IPv6: [fe80::a921:2d1f:61fb:8942]:50627 scope_id=6 >> Hello from UDP client!`<br/>`Echoed: Hello from UDP client!` |
| 5 | Ubuntu 22.04 (user) | Windows 11 (driver) | TCP | IPv4 | ДА | `[ECHO] Driver loaded (TCP:4444, UDP:4445)`<br/>`[TCP] Server ready on port 4444`<br/>`[TCP] Client connected`<br/>`[TCP] [192.168.68.129:38112] >> Hello from Linux user mode!(tcp)`<br/>`[TCP] Client disconnected` | `[Client] TCP IPv4 connecting to 192.168.68.128:4444`<br/>`Connected! Enter messages (type 'quit' to exit)`<br/>`> Hello from Linux user mdoe!(tcp)`<br/>`Sent: Hello from Linux user mdoe!(tcp)`<br/>`Echoed: Hello from Linux user mdoe!(tcp)` |
| 6 | Ubuntu 22.04 (user) | Windows 11 (driver) | UDP | IPv4 | ДА | `[ECHO] Driver loaded (TCP:4444, UDP:4445)`<br/>`[UDP] Server ready on port 4445`<br/>`[UDP] [192.168.68.129:8335] >> Hello from Linux user mode!(udp)` | `[Client] UDP IPv4 connecting to 192.168.68.128:4445`<br/> `UDP mode. Enter messages (type 'quit' to exit)`<br/>`> Hello from Linux user mode!(udp)`<br/>`Sent: Hello from Linux user mode!(udp)`<br/>`Echoed: Hello from Linux user mode!(udp)` |
| 7 | Windows 11 (driver) | Ubuntu 22.04 (user) | TCP | IPv4 | ДА | `[Client] Driver loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[TCP Client] Connecting to 192.168.68.129:4444`<br/>`[TCP Client] Connected to 192.168.68.129:4444`<br/>`[TCP Client] Sent 22 bytes: Hello from TCP client!`<br/>`[TCP Client] Received echo: Hello from TCP client!` | `[Server] TCP IPv4 on port 4444`<br/>`Listening for connections...`<br/>`[Client] IPv4: 192.168.68.128:49675`<br/>`Received: Hello from TCP client!`<br/>`Echoed: Hello from TCP client!`<br/>`[Client] Disconnected` |
| 8 | Windows 11 (driver) | Ubuntu 22.04 (user) | UDP | IPv6 | ДА | `[Client] Driver loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[UDP Client] Sending to fe80::70f:68b4:f07f:e3ef%9:4445: Hello from UDP client!`<br/>`[UDP Client] Sent 22 bytes`<br/>`[UDP Client] Received echo: Hello from UDP client!` | `[Server] UDP IPv6 on port 4445`<br/>`Waiting for datagrams...`<br/>`[Client] IPv6: [fe80::a921:2d1f:61fb:8942]:50646 scope_id=2 >> Hello from UDP client!`<br/>`[Debug] Link-local IPv6 detected, scope_id=2`<br/>`Echoed: Hello from UDP client!`|


## Результаты тестирования (Kernel mode - Kernel mode)

| № | Клиент | Сервер | Протокол | IP | Эхо получен | Вывод дебагера | Вывод dmesg |
|---|--------|--------|----------|----|-------------|------------------------|--|
| 9 | Ubuntu 22.04 (driver) | Windows 11 (driver) | TCP | IPv4 | ДА | `[ECHO] Driver loaded (TCP:4444, UDP:4445)`<br/>`[TCP] Server ready on port 4444`<br/>`[TCP] Client connected`<br/>`[TCP] [192.168.68.129:11438] >> Hello from TCP client!`<br/>`[TCP] Client disconnected`| `[Client] Module loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[Client] Starting test sequence to tcp server: 192.168.68.128`<br/>`[TCP Client] Connecting to 192.168.68.128:4444`<br/>`[TCP Client] Connected to 192.168.68.128:4444`<br/>`[TCP Client] Sent 22 bytes: Hello from TCP client!`<br/>`[TCP Client] Received echo: Hello from TCP client!`|
| 10 | Ubuntu 22.04 (driver) | Windows 11 (driver) | UDP | IPv6 | ДА | `[ECHO] Driver loaded (TCP:4444, UDP:4445)`<br/>`[UDP] Server ready on port 4445`<br/>`[UDP] [[fe80::70f:68b4:f07f:e3ef%9]:24477] >> Hello from UDP client!` | `[Client] Module loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[Client] Starting test sequence to udp server: fe80::a921:2d1f:61fb:8942%2`<br/>`[UDP Client] Sending to fe80::a921:2d1f:61fb:8942%2:4445: Hello from UDP client!`<br/>`[UDP Client] Sent 22 bytes`<br/>`[UDP Client] Received echo: Hello from UDP client!` |
| 11 | Windows 11 (driver) | Ubuntu 22.04 (driver) | TCP | IPv6 | ДА | `[Client] Driver loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[TCP Client] Connecting to fe80::70f:68b4:f07f:e3ef%9:4444`<br/>`[TCP Client] Connected to fe80::70f:68b4:f07f:e3ef%9:4444`<br/>`[TCP Client] Sent 22 bytes: Hello from TCP client!`<br/>`[TCP Client] Received echo: Hello from TCP client!` | `[TCP] Client connected`<br/>`[TCP] [[fe80::a921:2d1f:61fb:8942%2]:49674] >> Hello from TCP client!`<br/>`[TCP] Client disconnected` |
| 12 | Windows 11 (driver) | Ubuntu 22.04 (driver) | UDP | IPv4 | ДА | `[Client] Driver loaded - will send UDP to port 4445 and TCP to port 4444`<br/>`[UDP Client] Sending to 192.168.68.129:4445: Hello from UDP client!`<br/>`[UDP Client] Sent 22 bytes`<br/>`[UDP Client] Received echo: Hello from UDP client!` | `[UDP] [192.168.68.128:56938] >> Hello from UDP client!`|

## Вывод по результатам тестирования
В ходе тестирования было выполнено 12 перекрёстных конфигураций, охватывающих взаимодействие между user-mode и kernel-mode компонентами на платформах Windows 10, Windows 11 и Ubuntu 22.04. Все тесты завершились успешно — во всех сценариях эхо-ответ был получен, данные передавались без искажений, соединения устанавливались и корректно завершались.

**Ключевые результаты:**

1. **Кросс-платформенная совместимость** — KernelSocket API обеспечивает стабильную работу TCP/UDP при обмене данными между Windows и Linux как в конфигурациях user → kernel, kernel → user, так и в режиме прямого взаимодействия kernel ↔ kernel.

2. **Поддержка IPv6** — в ходе тестирования были успешно проверены сценарии с использованием IPv6 link-local адресов (с обязательным указанием scope ID для Linux и Windows). В частности, были отработаны связки Windows driver ↔ Windows user (IPv6), Windows driver ↔ Linux user (IPv6) и Windows driver ↔ Linux driver (IPv6). Это подтверждает корректную обработку scope ID в реализации API на обеих платформах.

3. **Работоспособность TCP и UDP** — оба протокола показали ожидаемое поведение: TCP с установлением соединения и гарантированной доставкой, UDP — с дейтаграммной моделью. Во всех случаях эхо-сервер возвращал отправленные данные без изменений.

4. **Стабильность в изолированной среде** — использование виртуального коммутатора vmnet1 в режиме host-only с отключённым брандмауэром на Windows-узлах позволило создать детерминированное окружение без влияния внешних сетевых факторов.

Таким образом, KernelSocket API успешно прошёл перекрёстное тестирование в заявленных конфигурациях и может быть рекомендован для использования в проектах, требующих единого сетевого интерфейса в режиме ядра для Windows и Linux.
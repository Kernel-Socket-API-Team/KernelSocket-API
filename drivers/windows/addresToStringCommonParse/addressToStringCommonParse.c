#include <ksockapi.h>

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrint("Test driver unload!\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = DriverUnload;

    net_error_t error;
    net_address_t addr_ipv4;
    addr_ipv4.family = NET_AF_INET4;
    addr_ipv4.addr.ipv4 = 0x0100007F; // 127.0.0.1

    const int bufferSize = 64;
    char* buffer = (char*)ExAllocatePool2(POOL_FLAG_NON_PAGED, bufferSize, 'bufC');

    error = net_address_to_string(&addr_ipv4, buffer, bufferSize, FALSE);

    DbgPrint("Address to string = %s\n", buffer);
    DbgPrint("Address to string length = %zu\n", strlen(buffer));
    DbgPrint("Address to string error = %d\n", error);

    net_address_t addr_ipv6;
    addr_ipv6.family = NET_AF_INET6;

    // Копируем IPv6 адрес: fd12:3456:789a::1
    uint8_t ipv6[16] = {
        0xFD, 0x12, // fd12
        0x34, 0x56, // 3456
        0x78, 0x9A, // 789a
        0x00, 0x00, // 0000
        0x00, 0x00, // 0000
        0x00, 0x00, // 0000
        0x00, 0x00, // 0000
        0x00, 0x01  // 0001
    };

    RtlCopyMemory(addr_ipv6.addr.ipv6, ipv6, 16);

    // Очищаем буфер перед новым тестом
    RtlZeroMemory(buffer, bufferSize);

    error = net_address_to_string(&addr_ipv6, buffer, bufferSize, FALSE);

    DbgPrint("Address to string = %s\n", buffer);
    DbgPrint("Address to string length = %zu\n", strlen(buffer));
    DbgPrint("Address to string error = %d\n", error);

    ExFreePoolWithTag(buffer, 'bufC');

    // Проверяем негативные случаи

    const int bufferSizeShort = 5;
    char* bufferShort = (char*)ExAllocatePool2(POOL_FLAG_NON_PAGED, bufferSize, 'bufS');

    error = net_address_to_string(&addr_ipv4, bufferShort, bufferSizeShort, FALSE);

    DbgPrint("Address to string = %s\n", bufferShort);
    DbgPrint("Address to string length = %zu\n", strlen(bufferShort));
    DbgPrint("Address to string error = %d\n", error);

    RtlZeroMemory(bufferShort, bufferSizeShort);

    error = net_address_to_string(&addr_ipv6, bufferShort, bufferSizeShort, FALSE);

    DbgPrint("Address to string = %s\n", bufferShort);
    DbgPrint("Address to string length = %zu\n", strlen(bufferShort));
    DbgPrint("Address to string error = %d\n", error);

    ExFreePoolWithTag(bufferShort, 'bufS');

    // Тестирование портов с универсальным буфером
    char* portBuffer = (char*)ExAllocatePool2(POOL_FLAG_NON_PAGED, NET_ADDRSTRLEN, 'prtB');

    uint16_t testPorts[] = {0, 1, 80, 65535};

    for (int i = 0; i < ARRAYSIZE(testPorts); i++)
    {
        uint16_t port = testPorts[i];
        addr_ipv4.port = port; // Предполагаем что пользователь забыл о конвертации порта
        addr_ipv6.port = RtlUshortByteSwap(port); // Тут конвертируем, для проверки что результат не меняется

        // Преобразование адреса с портом
        RtlZeroMemory(portBuffer, NET_ADDRSTRLEN);
        error = net_address_to_string(&addr_ipv4, portBuffer, NET_ADDRSTRLEN, TRUE);

        DbgPrint("Port test: %u | addr string='%s' | conv error=%d\n", port, portBuffer, error);

        addr_ipv6.port = port;

        // Преобразование адреса с портом
        RtlZeroMemory(portBuffer, NET_ADDRSTRLEN);
        error = net_address_to_string(&addr_ipv6, portBuffer, NET_ADDRSTRLEN, TRUE);

        DbgPrint("Port test: %u | addr string='%s' | conv error=%d\n", port, portBuffer, error);
    }

    // Освобождаем память
    ExFreePoolWithTag(portBuffer, 'prtB');

    return STATUS_SUCCESS;
}
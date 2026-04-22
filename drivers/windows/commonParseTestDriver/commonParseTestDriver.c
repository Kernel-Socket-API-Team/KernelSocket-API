#include <ksockapi.h>

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrint("Test driver unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = DriverUnload;

    net_address_t addr;

    // Комплексное тестирование в kernel режиме

    // Позитивные сценарии
    net_error_t err = net_address_parse("127.0.0.1", NET_AF_INET4, &addr);

    DbgPrint("ksockapi parse ip4 result = %d\n", err);

    // Перевод взят с сайта https://suip.biz/ru/?act=ip2hex и число дополнительно переведено в СЕТЕВОЙ порядок
    DbgPrint("ksockapi parse ip4 result = %d\n", addr.addr.ipv4 == 0x0100007F);

    err = net_address_parse("fd12:3456:789a::1", NET_AF_INET6, &addr);

    DbgPrint("ksockapi parse ip6 result = %d\n", err);

    // Перевод взят с сайта
    // https://ru.miniwebtool.com/%D0%BA%D0%BE%D0%BD%D0%B2%D0%B5%D1%80%D1%82%D0%B5%D1%80-ipv4ipv6-%D0%B2-%D1%88%D0%B5%D1%81%D1%82%D0%BD%D0%B0%D0%B4%D1%86%D0%B0%D1%82%D0%B5%D1%80%D0%B8%D1%87%D0%BD%D1%8B%D0%B9-%D1%84%D0%BE%D1%80%D0%BC%D0%B0%D1%82/?ip=fd12%3A3456%3A789a%3A%3A1
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
    int flag = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (ipv6[i] != addr.addr.ipv6[i])
        {
            flag = 1;
            break;
        }
    }

    if (flag == 0)
        DbgPrint("ksockapi parse ip6 result successfully\n");
    else
        DbgPrint("ksockapi parse ip6 result unsuccessfully\n");

    // Негативные сценраии, проверка ошибок
    err = net_address_parse("fd12:3456:789a::1", NET_AF_INET4, &addr);
    DbgPrint("IPv4 transmission but IPv6 format string = %d\n", err == NET_ERROR_INVALID_PARAM);

    err = net_address_parse("127.0.0.1", NET_AF_INET6, &addr);
    DbgPrint("IPv6 transmission but IPv4 format string = %d\n", err == NET_ERROR_INVALID_PARAM);

    err = net_address_parse("127.0.0.1", NET_AF_INET6, NULL);
    DbgPrint("NULL addr = %d\n", err == NET_ERROR_INVALID_PARAM);

    err = net_address_parse("127.0.0.1", -5, NULL);
    DbgPrint("transfer to unknown family = %d\n", err == NET_ERROR_INVALID_PARAM);

    return STATUS_SUCCESS;
}
#include "windows_common.h"

WSK_CONTEXT g_WskContext = {0};

WSK_CLIENT_DISPATCH WskAppDispatch = {
    MAKE_WSK_VERSION(1, 0), // Версия WSK 1.0
    0,                      // Зарезервировано
    NULL                    // ClientCallback (не используется)
};

net_error_t convert_status_from_windows(NTSTATUS ntstatus)
{
    if (NT_SUCCESS(ntstatus))
    {
        return NET_SUCCESS;
    }

    switch (ntstatus)
    {
    case STATUS_NO_MEMORY:
        return NET_ERROR_NO_MEMORY;
    case STATUS_ACCESS_DENIED:
        return NET_ERROR_ACCESS_DENIED;
    case STATUS_TIMEOUT:
        return NET_ERROR_TIMEOUT;
    case STATUS_BUFFER_TOO_SMALL:
        return NET_ERROR_BUFFER_TOO_SMALL;
    case STATUS_INVALID_PARAMETER:
        return NET_ERROR_INVALID_PARAM;
    default:
        return NET_ERROR_GENERIC;
    }
}

static NTSTATUS get_ifindex_by_name(const char* ifname, PULONG numeric_scope)
{
    ULONG index = 0;
    NTSTATUS status;
    UNICODE_STRING u_ifname;
    ANSI_STRING a_ifname;
    NET_LUID luid;
    MIB_IF_ROW2 row;

    if (!ifname || !numeric_scope)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Инициализируем ANSI строку
    RtlInitAnsiString(&a_ifname, ifname);

    // Инициализируем UNICODE_STRING (без буфера)
    RtlInitUnicodeString(&u_ifname, NULL);

    // Преобразуем ANSI в UNICODE с выделением буфера (TRUE)
    status = RtlAnsiStringToUnicodeString(&u_ifname, &a_ifname, TRUE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Получаем LUID по имени интерфейса
    status = ConvertInterfaceAliasToLuid(u_ifname.Buffer, &luid);
    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&u_ifname);
        return status;
    }

    // Получаем информацию об интерфейсе
    row.InterfaceLuid = luid;
    status = GetIfEntry2(&row);
    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&u_ifname);
        return status;
    }

    index = row.InterfaceIndex;
    *numeric_scope = index;

    RtlFreeUnicodeString(&u_ifname);
    return STATUS_SUCCESS;
}

NTSTATUS normalize_ipv6_scope_id(const char* str, char* output, SIZE_T output_size)
{
    char buffer[256];
    char* percent;
    char* bracket_end;
    char* open_bracket;
    char ifname[64];
    ULONG numeric_scope;
    BOOLEAN has_brackets = FALSE;

    if (!str || !output || output_size == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlStringCbCopyA(buffer, sizeof(buffer), str);

    // Проверяем, есть ли открывающая скобка
    open_bracket = strchr(buffer, '[');
    if (open_bracket == buffer)
    {
        has_brackets = TRUE;
    }

    // Ищем закрывающую скобку
    if (has_brackets)
    {
        bracket_end = strchr(buffer, ']');
        if (!bracket_end)
        {
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        bracket_end = buffer + strlen(buffer);
    }

    // Ищем % внутри скобок (или внутри всей строки, если скобок нет)
    percent = strchr(buffer, '%');
    if (percent && (!has_brackets || percent < bracket_end))
    {
        char* tmp = percent + 1;
        int i = 0;

        // Сохраняем то, что после %
        char* end_ptr = has_brackets ? bracket_end : (buffer + strlen(buffer));
        while (*tmp && tmp < end_ptr && i < 63)
        {
            ifname[i++] = *tmp++;
        }
        ifname[i] = '\0';

        // Проверяем, является ли строка числом
        BOOLEAN is_number = (i > 0);
        for (int j = 0; j < i && is_number; j++)
        {
            if (ifname[j] < '0' || ifname[j] > '9')
            {
                is_number = FALSE;
            }
        }

        if (is_number)
        {
            // Уже число, парсим его
            numeric_scope = 0;
            for (int j = 0; j < i; j++)
            {
                numeric_scope = numeric_scope * 10 + (ifname[j] - '0');
            }

            // Если не было скобок — добавляем
            if (!has_brackets)
            {
                int prefix_len = (int)(percent - buffer);
                RtlStringCbPrintfA(output, output_size, "[%.*s%%%lu%s]", prefix_len, buffer, numeric_scope, tmp);
                return STATUS_SUCCESS;
            }

            // Были скобки — копируем как есть
            RtlStringCbCopyA(output, output_size, buffer);
            return STATUS_SUCCESS;
        }

        // Это имя интерфейса — получаем индекс
        NTSTATUS status = get_ifindex_by_name(ifname, &numeric_scope);
        if (!NT_SUCCESS(status))
        {
            return STATUS_INVALID_PARAMETER;
        }

        // Перестраиваем строку с числовым scope_id
        char num_str[16];
        RtlStringCbPrintfA(num_str, sizeof(num_str), "%%%lu", numeric_scope);

        int prefix_len = (int)(percent - buffer);

        if (has_brackets)
        {
            // Были скобки — сохраняем их
            RtlStringCbCopyNA(output, output_size, buffer, prefix_len);
            RtlStringCbCatA(output, output_size, num_str);
            RtlStringCbCatA(output, output_size, tmp);
        }
        else
        {
            // Не было скобок — добавляем
            RtlStringCbPrintfA(output, output_size, "[%.*s%s%s]", prefix_len, buffer, num_str, tmp);
        }

        return STATUS_SUCCESS;
    }

    // Нет % — проверяем, нужно ли добавить скобки
    if (!has_brackets && strchr(buffer, ':') != NULL)
    {
        // Это IPv6 адрес без скобок — добавляем их
        RtlStringCbPrintfA(output, output_size, "[%s]", buffer);
        return STATUS_SUCCESS;
    }

    // Иначе копируем как есть (IPv4 или уже в скобках)
    RtlStringCbCopyA(output, output_size, buffer);
    return STATUS_SUCCESS;
}
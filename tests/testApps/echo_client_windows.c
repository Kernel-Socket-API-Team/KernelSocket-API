// echo_client_windows.c
// Компиляция в Developer PowerShell for VS: cl echo_client_windows.c ws2_32.lib /Fe:build\echo_client_windows.exe
// /Fo:build\echo_client_windows.obj Запуск:     echo_client_windows.exe <server_ip> <port> <tcp|udp> <ipv4|ipv6>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_storage server_addr;
    int port;
    int is_tcp;
    int is_ipv6;
    char* server_ip;

    // Проверка аргументов
    if (argc != 5)
    {
        printf("Usage: %s <server_ip> <port> <tcp|udp> <ipv4|ipv6>\n", argv[0]);
        printf("Examples:\n");
        printf("  %s 127.0.0.1 4444 tcp ipv4\n", argv[0]);
        printf("  %s ::1 4444 tcp ipv6\n", argv[0]);
        printf("  %s \"fe80::1%%6\" 4445 udp ipv6\n", argv[0]);
        return 1;
    }

    server_ip = argv[1];
    port = atoi(argv[2]);
    is_tcp = (strcmp(argv[3], "tcp") == 0);
    is_ipv6 = (strcmp(argv[4], "ipv6") == 0);

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Создание сокета
    if (is_tcp)
    {
        sock = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    else
    {
        sock = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (sock == INVALID_SOCKET)
    {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));

    if (is_ipv6)
    {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&server_addr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);

        // Парсинг IPv6 адреса (поддерживает scope_id)
        char ip_str[256];
        char* percent = strchr(server_ip, '%');

        if (percent)
        {
            size_t len = percent - server_ip;
            strncpy(ip_str, server_ip, len);
            ip_str[len] = '\0';
            addr6->sin6_scope_id = atoi(percent + 1);
        }
        else
        {
            strcpy(ip_str, server_ip);
            addr6->sin6_scope_id = 0;
        }

        if (inet_pton(AF_INET6, ip_str, &addr6->sin6_addr) != 1)
        {
            printf("Invalid IPv6 address: %s\n", server_ip);
            closesocket(sock);
            WSACleanup();
            return 1;
        }
    }
    else
    {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&server_addr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);

        if (inet_pton(AF_INET, server_ip, &addr4->sin_addr) != 1)
        {
            printf("Invalid IPv4 address: %s\n", server_ip);
            closesocket(sock);
            WSACleanup();
            return 1;
        }
    }

    printf("[Client] %s %s connecting to %s:%d\n", is_tcp ? "TCP" : "UDP", is_ipv6 ? "IPv6" : "IPv4", server_ip, port);

    // Подключение (для TCP) или просто запоминание адреса (для UDP)
    int addr_len = is_ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);

    if (is_tcp)
    {
        if (connect(sock, (struct sockaddr*)&server_addr, addr_len) == SOCKET_ERROR)
        {
            printf("Connect failed: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        printf("Connected! Enter messages (type 'quit' to exit)\n\n");
    }
    else
    {
        // Для UDP просто запоминаем адрес через connect (удобно для send/recv)
        connect(sock, (struct sockaddr*)&server_addr, addr_len);
        printf("UDP mode. Enter messages (type 'quit' to exit)\n\n");
    }

    // Цикл отправки/получения
    char buffer[BUFFER_SIZE];
    int bytes;

    while (1)
    {
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "quit") == 0)
            break;

        // Отправка
        if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR)
        {
            printf("Send failed: %d\n", WSAGetLastError());
            break;
        }
        printf("  Sent: %s\n", buffer);

        // Получение ответа
        bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
        {
            printf("Recv failed or connection closed\n");
            break;
        }

        buffer[bytes] = '\0';
        printf("  Echoed: %s\n", buffer);
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}
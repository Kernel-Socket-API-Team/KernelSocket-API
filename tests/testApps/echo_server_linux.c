// echo_server_linux.c
// Компиляция: gcc echo_server_linux.c -o ./build/echo_server_linux
// Запуск:     ./echo_server_linux <port> <tcp|udp> <ipv4|ipv6>

#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[])
{
    int sock = -1;
    struct sockaddr_storage server_addr;
    int port;
    int is_tcp;
    int is_ipv6;

    // Проверка аргументов
    if (argc != 4)
    {
        printf("Usage: %s <port> <tcp|udp> <ipv4|ipv6>\n", argv[0]);
        printf("Examples:\n");
        printf("  %s 4444 tcp ipv4\n", argv[0]);
        printf("  %s 4445 udp ipv6\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    is_tcp = (strcmp(argv[2], "tcp") == 0);
    is_ipv6 = (strcmp(argv[3], "ipv6") == 0);

    // Создание сокета
    if (is_tcp)
    {
        sock = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    }
    else
    {
        sock = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
    }

    if (sock < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    // Настройка адреса
    memset(&server_addr, 0, sizeof(server_addr));
    if (is_ipv6)
    {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&server_addr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_scope_id = 0;
    }
    else
    {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&server_addr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        addr4->sin_addr.s_addr = INADDR_ANY;
    }

    // Привязка
    socklen_t addr_len = is_ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    if (bind(sock, (struct sockaddr*)&server_addr, addr_len) < 0)
    {
        perror("Bind failed");
        close(sock);
        return 1;
    }

    printf("[Server] %s %s on port %d\n", is_tcp ? "TCP" : "UDP", is_ipv6 ? "IPv6" : "IPv4", port);

    if (is_tcp)
    {
        // TCP сервер
        if (listen(sock, SOMAXCONN) < 0)
        {
            perror("Listen failed");
            close(sock);
            return 1;
        }

        printf("Listening for connections...\n\n");

        while (1)
        {
            struct sockaddr_storage client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);

            if (client_sock < 0)
            {
                perror("Accept failed");
                continue;
            }

            // Вывод адреса клиента
            char client_ip[INET6_ADDRSTRLEN];
            if (client_addr.ss_family == AF_INET6)
            {
                struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&client_addr;
                inet_ntop(AF_INET6, &addr6->sin6_addr, client_ip, sizeof(client_ip));
                printf("[Client] IPv6: [%s]:%d scope_id=%u\n", client_ip, ntohs(addr6->sin6_port),
                       addr6->sin6_scope_id);
            }
            else
            {
                struct sockaddr_in* addr4 = (struct sockaddr_in*)&client_addr;
                inet_ntop(AF_INET, &addr4->sin_addr, client_ip, sizeof(client_ip));
                printf("[Client] IPv4: %s:%d\n", client_ip, ntohs(addr4->sin_port));
            }

            // Эхо-цикл
            char buffer[BUFFER_SIZE];
            int bytes;
            while ((bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0)
            {
                buffer[bytes] = '\0';
                printf("  Received: %s\n", buffer);
                send(client_sock, buffer, bytes, 0);
                printf("  Echoed: %s\n", buffer);
            }

            close(client_sock);
            printf("[Client] Disconnected\n---\n");
        }
    }
    else
    {
        // UDP сервер
        printf("Waiting for datagrams...\n\n");

        char buffer[BUFFER_SIZE];
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);

        while (1)
        {
            int bytes = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &addr_len);

            if (bytes < 0)
            {
                perror("recvfrom failed");
                continue;
            }

            buffer[bytes] = '\0';

            // Вывод адреса клиента
            char client_ip[INET6_ADDRSTRLEN];
            if (client_addr.ss_family == AF_INET6)
            {
                struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&client_addr;
                inet_ntop(AF_INET6, &addr6->sin6_addr, client_ip, sizeof(client_ip));
                printf("[Client] IPv6: [%s]:%d scope_id=%u >> %s\n", client_ip, ntohs(addr6->sin6_port),
                       addr6->sin6_scope_id, buffer);
            }
            else
            {
                struct sockaddr_in* addr4 = (struct sockaddr_in*)&client_addr;
                inet_ntop(AF_INET, &addr4->sin_addr, client_ip, sizeof(client_ip));
                printf("[Client] IPv4: %s:%d >> %s\n", client_ip, ntohs(addr4->sin_port), buffer);
            }

            // Отправка обратно
            sendto(sock, buffer, bytes, 0, (struct sockaddr*)&client_addr, addr_len);
        }
    }

    close(sock);
    return 0;
}
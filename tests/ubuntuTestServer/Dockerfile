FROM ubuntu:22.04

# Отключаем интерактивные диалоги
ENV DEBIAN_FRONTEND=noninteractive

# Устанавливаем все необходимые утилиты
RUN apt-get update && apt-get install -y \
    iputils-ping \
    net-tools \
    curl \
    wget \
    procps \
    lsof \
    iperf3 \
    netcat-traditional \
    netcat-openbsd \
    socat \
    dnsutils \
    tcpdump \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Открываем порты
EXPOSE 8080/tcp 8081/tcp 8082/tcp 8082/udp 5201/tcp 5201/udp
EXPOSE 9001/tcp 9002/tcp 9003/udp 9004/udp

#Протокол	Порт	Назначение      Команда сервера                                     Команда клиента (вводим текст + Ener, возвращает введёный текст)
#TCP4	    9001	TCP over IPv4   socat TCP4-LISTEN:9001,fork,reuseaddr EXEC:cat &    nc -4 localhost 9001
#TCP6	    9002	TCP over IPv6   socat TCP6-LISTEN:9002,fork,reuseaddr EXEC:cat &    nc -6 localhost 9002
#UDP4	    9003	UDP over IPv4   socat UDP4-LISTEN:9003,fork,reuseaddr EXEC:cat &    nc -4 -u localhost 9003
#UDP6	    9004	UDP over IPv6   socat UDP6-LISTEN:9004,fork,reuseaddr EXEC:cat &    nc -6 -u localhost 9004

#Тестирование с хоста в WSL
# TCP4
#echo "Hello from WSL TCP4" | nc -4 172.28.136.196 9001
# Ответ: Hello from WSL TCP4

# TCP6
#echo "Hello from WSL TCP6" | nc -6 ::1 9002
# или
#echo "Hello from WSL TCP6" | nc -6 localhost 9002

# UDP4
#echo "Hello from WSL UDP4" | nc -4 -u 172.28.136.196 9003

# UDP6
#echo "Hello from WSL UDP6" | nc -6 -u ::1 9004

#Тестирование с хоста Windows (вводим сообщение + Enter, должен вернуть введёный текст)
# TCP4
#ncat 172.28.136.196 9001

# TCP6
#ncat 172.28.136.196 9002

# UDP4
#ncat -u 172.28.136.196 9003

# UDP6
#ncat -u 172.28.136.196 9004

# Везде в примерах исопльзуется ip WSL который можно узнать запустив wsl и там написав:
# hostname -I

# Копируем entrypoint скрипт
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Запускаем entrypoint
ENTRYPOINT ["/entrypoint.sh"]
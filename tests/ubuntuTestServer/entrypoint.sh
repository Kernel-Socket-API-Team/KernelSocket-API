#!/bin/bash

# Запускаем все 4 эхо-сервера
socat TCP4-LISTEN:9001,fork,reuseaddr EXEC:cat &
socat TCP6-LISTEN:9002,fork,reuseaddr EXEC:cat &
socat UDP4-LISTEN:9003,fork,reuseaddr EXEC:cat &
socat UDP6-LISTEN:9004,fork,reuseaddr EXEC:cat &

# СОЗДАЁМ ОТДЕЛЬНЫЙ ФАЙЛ С АЛИАСАМИ
cat > /root/.monitor_aliases << 'EOF'
alias monitor-udp4='tcpdump -i any -n udp port 9003 -A'
alias monitor-udp6='tcpdump -i any -n udp port 9004 -A'
alias monitor-tcp4='tcpdump -i any -n tcp port 9001 -A'
alias monitor-tcp6='tcpdump -i any -n tcp port 9002 -A'
alias monitor-all='tcpdump -i any -n "port 9001 or port 9002 or port 9003 or port 9004" -A'
EOF

# Добавляем загрузку в .bashrc (для интерактивных сессий)
echo "source /root/.monitor_aliases" >> ~/.bashrc

# Применяем для текущей сессии скрипта
source /root/.monitor_aliases

# Держим контейнер запущенным
tail -f /dev/null
NAME=debug-server
NET=debug-network
IP=172.20.0.10

.PHONY: build run network stop shell logs clean monitor-tcp4 monitor-tcp6 monitor-udp4 monitor-udp6

all: build run

build:
	docker build -t net-debug .

network:
	docker network create --subnet=172.20.0.0/16 $(NET) 2>/dev/null || true

run: network
	docker run -it -d --name debug-server \
		-p 9001:9001/tcp \
		-p 9002:9002/tcp \
		-p 9003:9003/udp \
		-p 9004:9004/udp \
		net-debug

stop:
	-docker stop $(NAME)
	-docker rm $(NAME)

shell:
	docker exec -it $(NAME) bash

logs:
	docker logs -f $(NAME)

clean: stop
	-docker rmi net-debug
	-docker network rm $(NET)
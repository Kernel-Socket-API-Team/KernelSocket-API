# Имя Docker-образа и контейнера
IMAGE_NAME = kernelsocket-builder
CONTAINER_NAME = ks-build

# Версия ядра, необходимо синхронизировать с версией которая будет на ВМ!!!
KERNEL_VERSION := 6.8.0-110-generic

# Текущая директория (монтируется в контейнер)
WORKSPACE = $(realpath $(CURDIR)/../../../)

KDIR := /usr/src/linux-headers-$(KERNEL_VERSION)

.PHONY: build-image
build-image:
	docker build \
		--build-arg KERNEL_VERSION=$(KERNEL_VERSION) \
		-t $(IMAGE_NAME) .

.PHONY: run
run:
	docker run --rm -it \
		--name $(CONTAINER_NAME) \
		-v $(WORKSPACE):/workspace \
		$(IMAGE_NAME) /bin/bash

.PHONY: help
help:
	@echo "Доступные команды Makefile:"
	@echo "  build-image      - сборка Docker-образа"
	@echo "  run              - запуск интерактивного контейнера"
	@echo "  help             - показать эту справку"

.DEFAULT_GOAL := help
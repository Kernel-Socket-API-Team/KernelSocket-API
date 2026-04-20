# ===================== ВНИМАНИЕ ===================== 
# Данный Makefile не актуален и оставлен только для 
# того чтобы проверить сборку udp драйвера, то есть
# основным образом для совместимости. Поэтом в ближайшем будущем
# данный файл будет удален!!!!!


# Имя Docker-образа и контейнера
IMAGE_NAME = kernelsocket-builder
CONTAINER_NAME = ks-build

# Версия ядра, необходимо синхронизировать с версией которая будет на ВМ!!!
KERNEL_VERSION := 6.8.0-107-generic

# Текущая директория (монтируется в контейнер)
WORKSPACE = $(realpath $(CURDIR)/../..)
BUILD_TYPE ?= tcp_client

KDIR := /usr/src/linux-headers-$(KERNEL_VERSION)
SRC  := $(WORKSPACE)/drivers/linux/drivers/$(BUILD_TYPE)
BUILD := $(WORKSPACE)/build/linux/$(BUILD_TYPE)
TMP := $(WORKSPACE)/.module_build

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
		-w /workspace/drivers/linux/drivers \
		$(IMAGE_NAME) /bin/bash

.PHONY: help
help:
	@echo "Доступные команды Makefile:"
	@echo "  build-image      - сборка Docker-образа"
	@echo "  run              - запуск интерактивного контейнера"
	@echo "  help             - показать эту справку"
	@echo "  help             - показать эту справку"

# По умолчанию
.DEFAULT_GOAL := help
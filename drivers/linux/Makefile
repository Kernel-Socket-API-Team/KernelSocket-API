# Имя Docker-образа и контейнера
IMAGE_NAME = kernelsocket-builder
CONTAINER_NAME = ks-build

# Версия ядра, необходимо синхронизировать с версией которая будет на ВМ!!!
KERNEL_VERSION := 6.8.0-110-generic

# Текущая директория (монтируется в контейнер)
WORKSPACE = $(realpath $(CURDIR)/../..)

OUT_DIR := ../../../../build/linux
KDIR := /usr/src/linux-headers-$(KERNEL_VERSION)
TMP := $(WORKSPACE)/.module_build

# Список всех папок с драйверами (через пробел)
DRIVERS = serverTestDriver clientTestDriver

DRIVERS_PATH = drivers/linux/drivers

# Собрать вообще всё
.PHONY: build-all
build-all: build-image
	@for dir in $(DRIVERS); do \
		printf "\n\n--- Сборка драйвера в папке: $$dir ---\n\n\n"; \
		docker run --rm \
			-v $(WORKSPACE):/workspace \
			-w /workspace/$(DRIVERS_PATH)/$$dir \
			$(IMAGE_NAME) \
			make KERNEL_VERSION=$(KERNEL_VERSION) KDIR=/usr/src/linux-headers-$(KERNEL_VERSION); \
	done

# Очистить всё
.PHONY: clean-all
clean-all:
	rm -rf $(OUT_DIR)

.PHONY: build-server
build-server: build-image
	docker run --rm \
		-v $(WORKSPACE):/workspace \
		-w /workspace/drivers/linux/drivers/serverTestDriver \
		$(IMAGE_NAME) \
		make KERNEL_VERSION=$(KERNEL_VERSION) KDIR=/usr/src/linux-headers-$(KERNEL_VERSION)

.PHONY: build-client
build-client: build-image
	docker run --rm \
		-v $(WORKSPACE):/workspace \
		-w /workspace/drivers/linux/drivers/clientTestDriver \
		$(IMAGE_NAME) \
		make KERNEL_VERSION=$(KERNEL_VERSION) KDIR=/usr/src/linux-headers-$(KERNEL_VERSION)

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
	@echo "  build-server	  - запуск сборки serverTestDriver"
	@echo "  build-client     - запуск сборки clientTestDriver"
	@echo "  build-all        - запуск сборки всего (DRIVERS)"
	@echo "  clean-all        - запуск удаления .ko файлов из build"
	@echo "  help             - показать эту справку"

# По умолчанию
.DEFAULT_GOAL := help
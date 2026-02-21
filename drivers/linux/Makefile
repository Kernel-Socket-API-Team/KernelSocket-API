# Имя Docker-образа и контейнера
IMAGE_NAME = kernelsocket-builder
CONTAINER_NAME = ks-build

# Версия ядра, необходимо синхронизировать с версией которая будет на ВМ!!!
KERNEL_VERSION := 6.5.0-41-generic

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
		-w /workspace/drivers/linux \
		$(IMAGE_NAME) /bin/bash

.PHONY: build
build:
	rm -rf $(TMP)
	mkdir -p $(TMP)
	mkdir -p $(BUILD)

	cp -r $(SRC)/* $(TMP)/
	make -C $(KDIR) M=$(TMP) modules
	mv $(TMP)/*.ko $(BUILD)/

	rm -rf $(TMP)

.PHONY: build-tcp
build-tcp:
	$(MAKE) build BUILD_TYPE=tcp_client

.PHONY: build-udp
build-udp:
	$(MAKE) build BUILD_TYPE=udp_client

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: clean-docker
clean-docker:
	docker rmi $(IMAGE_NAME) 2>/dev/null || true


.PHONY: help
help:
	@echo "Доступные команды Makefile:"
	@echo "  build-image      - сборка Docker-образа"
	@echo "  run              - запуск интерактивного контейнера"
	@echo "  clean-docker     - полная очистка Docker"
	@echo "  help             - показать эту справку"
	@echo "  build-tcp        - сборка драйвера для теста tcp"
	@echo "  build-udp        - сборка драйвера для теста udp"
	@echo "  build            - сборка драйвера (по умолчанию для теста tcp)"
	@echo "  clean            - очистка файлов сборки"
	@echo "  clean-docker     - полная очистка Docker"
	@echo "  help             - показать эту справку"

# По умолчанию
.DEFAULT_GOAL := help
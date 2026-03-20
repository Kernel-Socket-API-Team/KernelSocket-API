# Makefile для сборки библиотеки ksockapi (использует kbuild)

# Определяем версию ядра
KERNEL_VERSION ?= $(shell uname -r)
KDIR := /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)

# Временная директория сборки
BUILD_DIR := $(PWD)/build

# Создаём временный модуль
MODULE_NAME := ksockapi_lib
obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := \
    src/common/linux/linux_common_release.o \
    src/dispatcher/dispatcher.o \
    src/adapters/linux/linux_adapter.o \
    src/common/linux/linux_common.o

# Флаги компиляции
ccflags-y := -I$(PWD)/../include
ccflags-y += -I$(PWD)/../src
ccflags-y += -Wno-error -Wno-unused-parameter

all:
	mkdir -p $(BUILD_DIR)
	
	# Копируем ВСЮ структуру src (сохраняя иерархию)
	cp -r ../src $(BUILD_DIR)/
	
	# Копируем заголовки
	cp -r ../include $(BUILD_DIR)/
	
	# Копируем этот Makefile в build
	cp $(PWD)/Makefile $(BUILD_DIR)/Makefile
	
	# Компилируем через kbuild
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) modules
	
	# Забираем готовый объектный файл
	cp $(BUILD_DIR)/$(MODULE_NAME).o ksockapi.o
	
	@echo "✅ Библиотека ksockapi.o готова"

clean:
	rm -rf $(BUILD_DIR) ksockapi.o

.PHONY: all clean
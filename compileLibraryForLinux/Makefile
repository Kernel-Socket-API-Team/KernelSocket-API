KDIR := /usr/src/linux-headers-6.8.0-101-generic
PWD := $(shell pwd)

BUILD_DIR := $(PWD)/build

build:
	mkdir -p $(BUILD_DIR)/src

	# ✅ Копируем ВСЮ структуру (это ключ)
	cp -r ../src/* $(BUILD_DIR)/src/
	cp -r ../include $(BUILD_DIR)/

	# Kbuild Makefile
	echo "obj-m += ksockapi.o" > $(BUILD_DIR)/Makefile
	echo "ksockapi-objs := \\" >> $(BUILD_DIR)/Makefile
	echo "    src/common/linux/linux_common_release.o \\" >> $(BUILD_DIR)/Makefile
	echo "    src/dispatcher/dispatcher.o \\" >> $(BUILD_DIR)/Makefile
	echo "    src/adapters/linux/linux_adapter.o \\" >> $(BUILD_DIR)/Makefile
	echo "    src/common/linux/linux_common.o" >> $(BUILD_DIR)/Makefile
	echo "" >> $(BUILD_DIR)/Makefile
	echo "ccflags-y += -I\$$(src)/include" >> $(BUILD_DIR)/Makefile
	echo "ccflags-y += -I\$$(src)/src" >> $(BUILD_DIR)/Makefile

	echo "ccflags-y += -O0" >> $(BUILD_DIR)/Makefile
	echo "ccflags-y += -fno-omit-frame-pointer" >> $(BUILD_DIR)/Makefile

	# Сборка
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) modules

	# Забираем результат
	cp $(BUILD_DIR)/ksockapi.o .
	cp $(BUILD_DIR)/.*.cmd . 2>/dev/null || :

	# Чистим build
	# rm -rf $(BUILD_DIR)

	@echo "✔ Built: ksockapi.o"

clean:
	rm -f ksockapi.o

.PHONY: build clean
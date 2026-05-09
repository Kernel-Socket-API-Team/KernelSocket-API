# Для Linux сборки
# В ~/tests/ubuntuTestServer лежит докер образ для создания сборочного контейнера
# но перед make run, в контейнере необходимо также задать версию KERNEL_VERSION 

KERNEL_VERSION := 6.8.0-110-generic
KDIR := /lib/modules/$(KERNEL_VERSION)/build
CUR_DIR := $(shell pwd)
OUT_DIR := ./build/
TMP_DIR := $(CUR_DIR)/../tmp_module

obj-m += echo-server-linux.o

echo-server-linux-y := echo_server_linux.o ../../../compileLibraryForLinux/ksockapi_sources.o 

# Включение заголовков
ccflags-y += -I../../../KernelSocket-API/include 
ccflags-y += -I../../../KernelSocket-API/compileLibraryForLinux/src

all:
	$(MAKE) -C $(KDIR) M="$(CUR_DIR)" modules
	mkdir -p $(TMP_DIR)
	cp $(CUR_DIR)/echo-server-linux.ko $(TMP_DIR)/
	$(MAKE) -C $(KDIR) M="$(CUR_DIR)" clean
	mkdir -p $(OUT_DIR)
	mv $(TMP_DIR)/echo-server-linux.ko $(OUT_DIR)/
	rmdir $(TMP_DIR) 2>/dev/null || true

clean:
	$(MAKE) -C $(KDIR) M="$(CUR_DIR)" clean
	rm -rf $(OUT_DIR)

.PHONY: all clean
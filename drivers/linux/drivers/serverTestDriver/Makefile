
KERNEL_VERSION := $(shell uname -r)
KDIR := /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)
OUT_DIR := ../../../../build/linux

obj-m += serverTestDriver.o

serverTestDriver-y := \
    driver.o \
    ../../../../compileLibraryForLinux/ksockapi_sources.o

ccflags-y += -I$(src)/../../../compileLibraryForLinux/include
ccflags-y += -I$(src)/../../../compileLibraryForLinux/src

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

	mkdir -p $(OUT_DIR)
	cp $(PWD)/serverTestDriver.ko $(OUT_DIR)/
	$(MAKE) -C $(KDIR) M=$(PWD) clean

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf $(OUT_DIR)
KERNEL_VERSION := 6.8.0-101-generic
KDIR := /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)
OUT_DIR := ../../../../build/linux

obj-m += accessFuntionsCheck.o

accessFuntionsCheck-y := driver.o ../../../../compileLibraryForLinux/ksockapi_sources.o 

ccflags-y += -I../../../../KernelSocket-API/include 
ccflags-y += -I../../../../KernelSocket-API/compileLibraryForLinux/src

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

	mkdir -p $(OUT_DIR)
	cp $(PWD)/accessFuntionsCheck.ko $(OUT_DIR)/
	$(MAKE) -C $(KDIR) M=$(PWD) clean

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf $(OUT_DIR)
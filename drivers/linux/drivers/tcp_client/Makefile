KERNEL_VERSION := 6.8.0-101-generic
KDIR := /lib/modules/$(KERNEL_VERSION)/build

PWD := $(shell pwd)

obj-m += driverCheckSend.o

driverCheckSend-y := driver.o \
    ../../../../compileLibraryForLinux/ksockapi_sources.o \

ccflags-y += -I ../../../../KernelSocket-API/include
ccflags-y += -I ../../../../KernelSocket-API/compileLibraryForLinux/src

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
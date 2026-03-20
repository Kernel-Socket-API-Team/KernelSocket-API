KERNEL_VERSION ?= $(shell uname -r)
KDIR := /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)

obj-m += driverCheckCompileModule.o

driverCheckCompileModule-y := \
    main.o \
    ../../../../compileLibraryForLinux/ksockapi_sources.o

ccflags-y += -I../../../../compileLibraryForLinux/include
ccflags-y += -I../../../../compileLibraryForLinux/src
ccflags-y += -fno-omit-frame-pointer

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) cleanobj-m += driverCheckCompileModule.o
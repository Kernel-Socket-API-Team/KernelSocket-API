KDIR := /usr/src/linux-headers-6.8.0-101-generic
PWD := $(shell pwd)

obj-m += driverCheckCompileModule.o # имя объектного фала должно совпадать с файлом у которого разрешение .c

# линкуемся с библиотекой
driverCheckCompileModule-objs := ../../../../compileLibraryForLinux/ksockapi.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
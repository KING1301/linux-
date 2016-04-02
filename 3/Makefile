
ifneq ($(KERNELRELEASE),)

obj-m:=my_cdev.o
else

KERNELDIR:=/lib/modules/$(shell uname -r)/build

PWD:=$(shell pwd)

default:

	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:

	rm -rf *.o *.mod.c *.mod.o *.ko module* Module*

endif

EXEC=device
all:$(EXEC)
$(EXEC):$(EXEC).c
	gcc -o$@ $<

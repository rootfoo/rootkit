
# Assume the source tree is where the running kernel was built
# You should set KERNELDIR in the environment if it's elsewhere
KROOT ?= /lib/modules/$(shell uname -r)/build
#export KROOT=/lib/modules/4.4.0-124-generic/build

# The current directory is passed to sub-makes as argument
PWD := $(shell pwd)

obj-m += rootkit.o

allofit: modules

modules:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

kernel_clean:
	@$(MAKE) -C $(KROOT) M=$(PWD) clean

clean: kernel_clean
	rm -rf Module.symvers modules.order rootkit.o.ur-safe


.PHONY: modules modules_install clean

ifneq ($(KERNELRELEASE),)
# Kbuild part of makefile
obj-m := onboard_io.o
else

# kernel sources

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M="$$PWD"

clean:
	$(MAKE) -C $(KDIR) M="$$PWD" $@

%.i %.s : %.c
	$(ENV_CROSS) \
	$(MAKE) -C $(KDIR) M=$$PWD $@
endif

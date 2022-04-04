ifneq ($(KERNELRELEASE),)
obj-m := hrt.o timer.o simple_wq.o
else

default:
	$(MAKE) -C $(KDIR) M="$$PWD"

clean:
	$(MAKE) -C $(KDIR) M=$$PWD $@

endif

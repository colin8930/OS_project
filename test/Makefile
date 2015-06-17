#
#makefile by colin <colin8930 AT gmail.com>
#
obj-m       += test.o
PWD=$(shell pwd)
 
all:
	$(MAKE) -C /lib/modules/3.16.0/build M=$(PWD) V=1 modules

clean:
	$(MAKE) -C /lib/modules/3.16.0/build M=$(PWD) clean

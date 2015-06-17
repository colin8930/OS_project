#
#makefile by colin(Zung-Ying Shen) <colin8930 AT gmail.com>
#
obj-m       += char_dev.o
PWD=$(shell pwd)
 
all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) V=1 modules
	gcc testing.c

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

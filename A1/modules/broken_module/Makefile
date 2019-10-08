# REFERENCE https://www.tldp.org/LDP/lkmpg/2.6/lkmpg.pdf

obj-m += broken-module.o
dir = /home/cs469/linux/debian/hdrtmp/usr/src/linux-headers-5.3.0+/

all:
	make -C $(dir) M=$(PWD) modules
clean:
	make -C $(dir) M=$(PWD) clean

load:
	sudo /usr/sbin/insmod broken-module.ko

unload:
	sudo /usr/sbin/rmmod broken-module.ko


# REFERENCE https://www.tldp.org/LDP/lkmpg/2.6/lkmpg.pdf

obj-m += simple_module.o
dir = /home/cs469/linux/debian/hdrtmp/usr/src/linux-headers-5.3.0+/

all:
	make -C $(dir) M=$(PWD) modules
clean:
	make -C $(dir) M=$(PWD) clean

load:
	sudo /usr/sbin/insmod simple_module.ko

unload:
	sudo /usr/sbin/rmmod simple_module.ko


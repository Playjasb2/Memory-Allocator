# REFERENCE https://www.tldp.org/LDP/lkmpg/2.6/lkmpg.pdf

obj-m += simple_args.o
dir = /home/cs469/linux/debian/hdrtmp/usr/src/linux-headers-5.3.0+/

all:
	make -C $(dir) M=$(PWD) modules
clean:
	make -C $(dir) M=$(PWD) clean


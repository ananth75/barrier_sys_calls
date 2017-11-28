CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
IOT_HOME = /opt/iot-devkit/1.7.2/sysroots
PATH := $(PATH):$(IOT_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux
SROOT=$(IOT_HOME)/i586-poky-linux
all:
	i586-poky-linux-gcc -pthread -o main main.c --sysroot=$(SROOT) -Wall
clean:
	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -C $(SROOT)/usr/src/kernel M=$(PWD) clean
	rm -f *.o

#obj-m += ovc4_kernel_module.o
obj-m += src/ovc4_camera_driver.o
#ovc4_kernel_module-objs := ./src/ovc4_kernel_module.o ./src/ovc4_camera_driver.o

EXTRA_CFLAGS=-I$(PWD)/include -I/usr/src/linux-headers-4.9.140-tegra-ubuntu18.04_aarch64/nvidia/include/

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


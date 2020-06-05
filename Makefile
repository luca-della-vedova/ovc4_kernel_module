obj-m += ovc4_kernel_module.o
ovc4_kernel_module-objs := ./src/ovc4_kernel_module.o ./src/ovc4_camera_driver.o

EXTRA_CFLAGS=-I$(PWD)/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


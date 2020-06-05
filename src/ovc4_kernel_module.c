
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */

#include <ovc4_driver/ovc4_camera_driver.h>

// TODO multiple cameras (and different?)

static char *sensor_name = "null";
module_param(sensor_name, charp, 0);
MODULE_PARM_DESC(sensor_name, "Name of the sensor to connect");


int init_module(void)
{
	printk(KERN_INFO "Hello world 1.\n");

	printk(KERN_INFO "Configuring sensor %s\n", sensor_name);

	camera_driver_test();

	return 0;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Goodbye world 1.\n");
}

MODULE_DESCRIPTION("Driver for Open Vision Computer 4 camera modules");
MODULE_AUTHOR("Open Source Robotics Corporation");
MODULE_LICENSE("GPL v2");

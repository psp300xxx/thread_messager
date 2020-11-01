#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
// #include <linux/device.h>
#include <linux/version.h>
#include <linux/hashtable.h>
#include <linux/uaccess.h>
#include "thread_manager_spowner.h"

#include <asm-generic/errno-base.h>
#include "ioctl_switch_functions.h"
MODULE_AUTHOR("Luigi De Marco <demarco.1850504@studenti.uniroma1.it>");
MODULE_DESCRIPTION("ioctl example");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

long mydev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int mydev_open(struct inode *inode, struct file *filp);
int mydev_release(struct inode *inode, struct file *filp);
struct class * dev_cl_group =NULL;
int major_secondary = -1;

// Variables to correctly setup/shutdown the pseudo device file
static int major;
static struct class *dev_cl = NULL;
static struct device *device = NULL;


// Variables configurable from /sys valid for the entire module
int bytes_per_message = 100;
module_param(bytes_per_message, int, S_IRUGO|S_IWUSR);
int total_bytes_in_queue = 100 * 4000 ;
module_param(total_bytes_in_queue, int , S_IRUGO|S_IWUSR);



/// File operations for the module
struct file_operations fops = {
	open: mydev_open,
	unlocked_ioctl: mydev_ioctl,
	compat_ioctl: mydev_ioctl,
	release: mydev_release
};



long mydev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	groupt info;
	switch (cmd) {
		case IOCTL_INSTALL_GROUP_T:
			copy_from_user(&info, (groupt *) arg, sizeof(groupt));
			ret = set_new_driver(info.group);
			goto out;
	}

    out:
	return ret;
}




int mydev_open(struct inode *inode, struct file *filp) {
	return 0;
}


int mydev_release(struct inode *inode, struct file *filp)
{

	return 0;
}


static int __init mydev_init(void)
{
	// hash_init(hash_table);
	int err;
	major = register_chrdev(0, DRIVER_NAME, &fops);
	// Dynamically allocate a major for the device
	if (major < 0) {
		printk(KERN_ERR "%s: Failed registering char device\n", DRIVER_NAME);
		err = major;
		goto first_fail;
	}

	// Create a class for the device
	dev_cl = class_create(THIS_MODULE, "mydev");
	if (IS_ERR(dev_cl)) {
		printk(KERN_ERR "%s: failed to register device class\n", DRIVER_NAME);
		err = PTR_ERR(dev_cl);
		goto failed_classreg1;
	}

	dev_cl_group = class_create(THIS_MODULE, "GROUP_MSG_MNGR");
	if (IS_ERR(dev_cl_group)) {
		printk(KERN_ERR "%s: failed to register device class group mgr\n", DRIVER_NAME);
		err = PTR_ERR(dev_cl_group);
		goto failed_classreg2;
	}
	printk(KERN_ERR "ok registered group class\n");

	// Create a device in the previously created class
	device = device_create(dev_cl, NULL, MKDEV(major, 0), NULL, DRIVER_NAME);
	if (IS_ERR(device)) {
		printk(KERN_ERR "%s: failed to create device\n", DRIVER_NAME);
		err = PTR_ERR(device);
		goto failed_devreg;
	}

	printk(KERN_INFO "%s: special device registered with major number %d\n", DRIVER_NAME, major);

	return 0;

failed_devreg:
	class_unregister(dev_cl_group);
	class_destroy(dev_cl_group);
failed_classreg2:
	class_unregister(dev_cl);
	class_destroy(dev_cl);
failed_classreg1:
	unregister_chrdev(major, DRIVER_NAME);
first_fail:
	return err;
}

static void __exit mydev_exit(void)
{
	device_destroy(dev_cl, MKDEV(major, 0));
	class_unregister(dev_cl);
	class_destroy(dev_cl);
	unregister_chrdev(major, DRIVER_NAME);
	unregister_and_destroy_all_devices();
}


module_init(mydev_init)
module_exit(mydev_exit)

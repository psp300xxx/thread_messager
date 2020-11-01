#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/rwsem.h>
#include "ioctl_switch_functions.h"

// #include "thread_manager_spowner.h"
#include "../group_message_manager/group_message_manager.h"
#include <linux/kdev_t.h>
#include <linux/types.h>


struct device *device_group = NULL;
EXPORT_SYMBOL(device_group);

// Driver name for the newly created group
char * driver_name = NULL;
int major_number;
static dev_t  current_devt = 1;
// Devices created
static int number_devices = 0;
struct devices_created devices;
int list_devices_created =0;

// Semaphore protecting the list of semaphore created
static DECLARE_RWSEM(rw_semaphore);

int current_open(struct inode *inode, struct file *filp);

// called in exiting from module, I destroy all the group data and relative devices
void unregister_and_destroy_all_devices(){
		destroy_hashtable_data();
		struct devices_created * iter;
		char * current_driver_name;
		if( !list_devices_created ){
			return;
		}
		current_driver_name = kmalloc(sizeof(char)*20,0);
		list_for_each_entry(iter, &devices.list, list){
				sprintf(current_driver_name, DRIVER_NAME_NUMB, iter->group);
				device_destroy(dev_cl_group, iter->devt);
		}
		class_unregister(dev_cl_group);
		class_destroy(dev_cl_group);
		list_for_each_entry(iter, &devices.list, list){
				sprintf(current_driver_name, DRIVER_NAME_NUMB, iter->group);
				unregister_chrdev(iter->major_number, current_driver_name);
		}
		kfree(current_driver_name);
}

long set_new_driver( int group ){
    int ret;
    struct devices_created * iter;
	// Checks if group already exists
	down_write(&rw_semaphore);
	if(list_devices_created){
		list_for_each_entry(iter, &devices.list, list){
				if(iter->group == group){
					up_write(&rw_semaphore);
					return 0;
				}
		}
	}
	// If not, I try to create a new one
	// And I add it into the list.
    ret = init_new_device(group);
    if (!ret){
        if (!list_devices_created){
            INIT_LIST_HEAD(&devices.list);
			list_devices_created=1;
        }
        ret = add_group_list(&devices, group, current_devt, major_number);
		if(ret<0){
			up_write(&rw_semaphore);
			return -1;
		}
		number_devices++;
		up_write(&rw_semaphore);
        return 0;
    }
	up_write(&rw_semaphore);
    return -1;
}

int add_group_list(struct devices_created * devs, int group, dev_t new_devt, int major){
	struct devices_created *tmp;
	tmp = (struct devices_created *)kmalloc(sizeof(struct devices_created), 0);
	if (tmp==NULL) {
		return -1;
	}
	tmp -> group = group;
	tmp -> devt = new_devt;
	tmp -> major_number = major;
	list_add_tail(&(tmp->list), &(devs->list));
    return group;
}



int current_open(struct inode *inode, struct file *filp) {

	return 0;
}



int  init_new_device(int group)
{
	int err;
	char * driver_name ;
	driver_name = kmalloc(sizeof(char) * 40,GFP_KERNEL );
	if(driver_name==NULL){
		return -ENOMEM;
	}
	sprintf(driver_name, DRIVER_NAME_NUMB, group);
	major_number = register_chrdev(0, driver_name, &file_ops_gmm_origin);

	// Dynamically allocate a major_number for the device
	if (major_number < 0) {
		printk(KERN_ERR "%s: Failed registering char device\n", driver_name);
		err = PTR_ERR(major_number);
		goto finish;
	}

	current_devt = MKDEV(major_number,0);

	device_group = device_create(dev_cl_group, NULL, current_devt, NULL, driver_name);
	if (IS_ERR(device_group)) {
		pr_err("%s:%d error code %ld \n", __func__, __LINE__, PTR_ERR(device_group));
		err = PTR_ERR(device_group);
		goto failed_classreg;
	}

	printk(KERN_INFO "%s: special device registered with major_number number %d\n", driver_name, major_number);

	return 0;
failed_classreg:
	unregister_chrdev(major_number, driver_name);
finish:
	kfree(driver_name);
	return err;
}

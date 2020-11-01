#include <linux/list.h>
#include <linux/device.h>

long set_new_driver( int group );

#define DRIVER_NAME_NUMB "GROUP_MESSAGE_MANAGER%d"


struct devices_created {
    int group;
    dev_t devt;
    int major_number;
    struct list_head list;
};



void unregister_and_destroy_all_devices(void);

int add_group_list(struct devices_created * devs, int group, dev_t new_devt, int major);


int init_new_device(int group);


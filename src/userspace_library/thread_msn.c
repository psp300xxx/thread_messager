#include "thread_msn.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define MAIN_DEVICE "/dev/thread_manager_spowner"

#define SUB_DEVICES "/dev/synch/GROUP_MESSAGE_MANAGER%d"

#define SLEEPING_TIME 0.1 //0.1 seconds

// this functions tries to open a group, if it exists it returns 
// a File Descriptor of the file we are operating to.
// If the group does not exists, the function tries to install it.
// if the operation fails, a number < 0 is returned.
int open_group(groupt * group_descriptor){
    int ret;
    int fd;
    int group_fd;
    int tries;
    char * group_to_open;
    fd = open(MAIN_DEVICE, O_RDWR);
	if(fd < 0) {
		perror("Error opening main device");
		return -1;
	}
    ret = ioctl(fd, IOCTL_INSTALL_GROUP_T , group_descriptor);
    if (ret<0){
        perror("Error installing new group");
        close(fd);
		return -1;
    }
    close(fd);
    group_to_open = malloc(sizeof(char)* strlen(SUB_DEVICES)*2);
    if(group_to_open==NULL){
        perror("mem error");
        return -1;
    }
    sprintf(group_to_open, SUB_DEVICES, group_descriptor->group);
    // Since I have to wait udev creates the symbolic links, I try to do it
    // 10 times before making the operation fail
    tries = 100;
    while(tries>=0){
        group_fd = open(group_to_open, O_RDWR);
        if(group_fd>0){
            goto end;
        }
        sleep(SLEEPING_TIME);
        tries--;
    }
end:
    free(group_to_open);
    return group_fd;
}

// // This function flushes all the delayed messages for the given group 
// int flush_group_messages(int file_descriptor){
//     int ret;
//     ret = flush(file_descriptor);
//     return ret;
// }


// tries to write a message into the group managed by the given file descriptor
// I assume message has been already allocated
int write_message(int file_descriptor, char * message, int max_length){
    int ret;
    ret = write(file_descriptor,message, max_length);
    return ret;
}

// read a message(if any) from the group managed by the given file descriptor
int read_message(int file_descriptor, char * buffer, int max_length){
    int ret;
    ret = read(file_descriptor,buffer, max_length);
    return ret;
}

long set_message_delay(int file_descriptor, long new_delay){
    long ret;
    ret = ioctl(file_descriptor,IOCTL_GMM_SET_DELAY, new_delay);
    return ret;
}

void close_group(int file_descriptor){
    close(file_descriptor);
}

// sets the thread having tid as gettid() in sleeping mode on the group managed by the device
// describer by the file descriptor
int sleep_tid(int file_descriptor, int tid){
    long ret;
    ret = ioctl(file_descriptor,IOCTL_GMM_SLEEP_TID, tid);
    return ret;
}

// As the previous func, but in reverse
int awake_tids(int file_descriptor){
    long ret;
    ret = ioctl(file_descriptor,IOCTL_GMM_AWAKE_TIDS);
    return ret;  
}
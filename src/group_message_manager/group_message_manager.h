#pragma once
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/timekeeping.h>
// #include "../common.h"
#include "../thread_manager_spowner/thread_manager_spowner.h"


#define QUEUE_EMPTY_MESSAGE "Error: Queue is empty\n"


//list that will contain a queue of messages
struct message_queue {
    thread_message * message;
    ktime_t publishing_time;
    struct list_head list;
};

// list containing the list of pids that are currently sleeping
struct sleeping_tid{
    pid_t tid;
    struct list_head list;
};


// Struct containing information for the group 
// managed by the tid.
typedef struct {
    unsigned long open_count;
    unsigned long control_number;
    ktime_t sending_delay;
    int msg_in_delivering;
    int msg_in_publishing;
    int major_number;
    struct message_queue delivering_queue;
    struct message_queue publishing_queue;
    struct sleeping_tid sleeping_tid_list;
    int number_of_sleeping_tid;
    struct rw_semaphore delivering_semaphore;
    struct rw_semaphore publishing_semaphore;
    struct rw_semaphore sleeping_tid_semaphore;
} node_information;



void destroy_map(void);


int gmm_open(struct inode *inode, struct file *filp);

ssize_t gmm_read(struct file * file, char * buffer, size_t lenght, loff_t * offset);

ssize_t gmm_write(struct file * file, const char __user * buffer, size_t lenght, loff_t * offset );

int gmm_release(struct inode * inode, struct file * filp);

int gmm_flush (struct file *, fl_owner_t id);

long gmm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

void destroy_hashtable_data(void);

// Since I declare the class into another file, I let the other file obtain
// the file_operation struct needed.
extern struct file_operations file_ops_gmm_origin;




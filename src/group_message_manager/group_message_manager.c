#include "group_message_manager.h"
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#include <linux/hashtable.h>
#include <linux/types.h>


#define CONTROL_NUMBER 633443


// Variable used to check if Hashtable has been initialized
static int table_initialized = 0;
static DEFINE_HASHTABLE(hash_table, 16);

// This functions checks if the tid given as parameters is in the sleeping tids.
int is_in_sleeping_tids(int tid, node_information * node_info);

// Node struct used to save data into the hashtable
struct h_node {
    void * data;
    struct hlist_node node_info;
};

void destroy_hashtable_data(void){
    struct h_node * curr;
    int bucket;
    node_information * current_node_info;
    struct message_queue * msg;
    bucket = 0;
    hash_for_each(hash_table, bucket, curr, node_info){
            current_node_info = curr->data;
            down_write(&current_node_info->publishing_semaphore);
            down_write(&current_node_info->delivering_semaphore);
            down_write(&current_node_info->sleeping_tid_semaphore);
            while(! list_empty(&current_node_info->publishing_queue.list ) ){
                msg = list_first_entry(&current_node_info->publishing_queue.list, struct message_queue, list);
                list_del(&msg->list);
                vfree(msg->message->text);
                kfree(msg->message);
            }
            while(! list_empty(&current_node_info->delivering_queue.list ) ){
                msg = list_first_entry(&current_node_info->delivering_queue.list, struct message_queue, list);
                list_del(&msg->list);
                vfree(msg->message->text);
                kfree(msg->message);
            }
            kfree(current_node_info);
            up_write(&current_node_info->publishing_semaphore);
            up_write(&current_node_info->delivering_semaphore);
            up_write(&current_node_info->sleeping_tid_semaphore);
    }
}

node_information * create_node_info(int major_number);

// creates a new node (Group Informations)
// Having the first values needed at the beginning
inline node_information * create_node_info(int major_number){
    node_information * node_info;
    node_info = kmalloc(sizeof(node_information), GFP_KERNEL);
    if(node_info==NULL){
        return NULL;
    }
    node_info -> control_number = CONTROL_NUMBER;
    init_rwsem(&node_info->delivering_semaphore);
    init_rwsem(&node_info->publishing_semaphore);
    init_rwsem(&node_info->sleeping_tid_semaphore);
    INIT_LIST_HEAD(&node_info->delivering_queue.list);
    INIT_LIST_HEAD(&node_info->publishing_queue.list);
    INIT_LIST_HEAD(&node_info->sleeping_tid_list.list);
    node_info->msg_in_delivering = 0;
    node_info->major_number = major_number;
    node_info->msg_in_publishing = 0;
    node_info ->number_of_sleeping_tid = 0;
    node_info -> open_count = 0;
    return node_info;
}

// Given the dev_t as input parameter, this function
// The struct relative to the group if present, 0 otherwise
node_information * get_device_data(dev_t i_ino){
    struct h_node * cur ;
    struct hlist_node * tmp;
    node_information * node_info;
    u32 key;
    key = (u32)MAJOR(i_ino);
    cur = NULL;
    node_info = NULL;
    tmp = NULL;
    hash_for_each_possible_safe(hash_table, cur,tmp, node_info, key) {
        node_info = (node_information *) cur->data;
        if(node_info->control_number == CONTROL_NUMBER && node_info->major_number == MAJOR(i_ino)){
            return node_info;
        }
    }
    return 0;
}

int is_in_sleeping_tids(int tid, node_information * node_info){
        struct sleeping_tid * sleeper;
        down_read(&node_info->sleeping_tid_semaphore);
        list_for_each_entry(sleeper, &node_info->sleeping_tid_list.list, list){
            if(sleeper->tid == tid){
                up_read(&node_info->sleeping_tid_semaphore);
                return 1;
            }
        }
        up_read(&node_info->sleeping_tid_semaphore);
        return 0;
}



// It's the open() syscall implementation
int gmm_open(struct inode *inode, struct file *filp){
    node_information * node_info;
    struct h_node * my_node;
    u32 key;
    key = (u32)MAJOR(inode->i_rdev);
    // INIT HASHTABLE, THIS IS THE FIRST TIME USER OPENS A DEVICE
    if(table_initialized<=0){
        hash_init(hash_table);
        table_initialized++;
    }
    node_info = get_device_data(inode->i_rdev);
    if ( node_info == NULL ){
        // creating new data for this newly created group
        node_info = create_node_info(MAJOR(inode->i_rdev));
        if(node_info==NULL){
            return -1;
        }
        my_node = kmalloc(sizeof(struct h_node), GFP_KERNEL);
        if(my_node==NULL){
            return -1;
        }
        my_node->data = node_info;
        hash_add(hash_table, &my_node->node_info, key);
    }
    // already inizialized structure for this device
    filp->private_data = node_info;
    return 0;
}

int gmm_release(struct inode * inode, struct file * filp){
    return 0;
}




// IOCTL function
long gmm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    long ret;
    long delay;
    node_information * node_info;
    struct sleeping_tid * sleeper;
    struct sleeping_tid ** sleepers_ptrs;
    int count = 0;
    ret = 0;
	switch (cmd) {
        // Sets the delay for the given group
		case IOCTL_GMM_SET_DELAY:
            delay = arg;
            node_info = filp->private_data;
			node_info -> sending_delay = delay;
			goto out;
        // set the given TID into sleeping mode, setting it into the sleeping state 
        case IOCTL_GMM_SLEEP_TID:
            node_info = filp -> private_data;
            down_write(&node_info->sleeping_tid_semaphore);
            sleeper = kmalloc(sizeof(struct sleeping_tid), GFP_KERNEL);
            if(sleeper==NULL){
                ret = -ENOMEM;
                goto out;
            }
            sleeper -> tid = arg;
            list_add_tail(&sleeper->list, &node_info->sleeping_tid_list.list);   
            node_info->number_of_sleeping_tid ++;
            up_write(&node_info->sleeping_tid_semaphore);
            goto out;
        // Awakes all the sleeping TIDS
        case IOCTL_GMM_AWAKE_TIDS:
            node_info = filp -> private_data;
            down_write(&node_info->sleeping_tid_semaphore);
            // I look for the current entry in sleeping threads.
            // The pointer is necessary due to the necessity of not modifying the 
            // list when we iterates on it.
            sleepers_ptrs = vmalloc(sizeof(struct sleeping_tid *)*(node_info->number_of_sleeping_tid));
            if(sleepers_ptrs==NULL){
                ret = -ENOMEM;
                goto out;
            }
            list_for_each_entry(sleeper, &node_info->sleeping_tid_list.list, list){
                sleepers_ptrs[count++] = sleeper;
            }
            for( ; count>0 ; count--){
                list_del(&sleepers_ptrs[count-1]->list);
            }
            node_info->number_of_sleeping_tid = 0;
            vfree(sleepers_ptrs);
            up_write(&node_info->sleeping_tid_semaphore);
            goto out;	
	}
    out:
	return ret;
}

// Gets the minimum message in publishing queue in which the delay has expired
// If no message is in this state, NULL is returned
inline struct message_queue * get_minimum_message(node_information * node_info){
    struct message_queue * curr;
    struct message_queue * min;
    curr = NULL;
    min = NULL;
    down_read(&node_info->publishing_semaphore);
    list_for_each_entry(curr, &node_info->publishing_queue.list, list){
			if(curr->publishing_time >0 && curr->publishing_time <= ktime_get_boottime()){
                    if (min==NULL){
                        min = curr;
                    }
                    else {
                        if(curr->publishing_time < min->publishing_time){
                            min = curr;
                        }
                    }
			}
	}
    up_read(&node_info->publishing_semaphore);
    return min; 
}

inline struct message_queue * get_minimum_message_no_constrains(node_information * node_info){
    struct message_queue * curr;
    struct message_queue * min;
    curr = NULL;
    min = NULL;
    list_for_each_entry(curr, &node_info->publishing_queue.list, list){
			if(curr->publishing_time >0){
                    if (min==NULL){
                        min = curr;
                    }
                    else {
                        if(curr->publishing_time < min->publishing_time){
                            min = curr;
                        }
                    }
			}
	}
    return min; 
}

// Flushing system call
// All messages in publishing queue are inserted into delivering queue.
// In order of publishing time
int gmm_flush (struct file * filp, fl_owner_t id){
    node_information * node_info;
    struct message_queue * curr;
    node_info = filp->private_data;
    down_write(&node_info->publishing_semaphore);
    down_write(&node_info->delivering_semaphore);
    while( (curr =  get_minimum_message_no_constrains(node_info)) != NULL ){
        list_del(&curr->list);
        list_add_tail(&curr->list, &node_info->delivering_queue.list);
        node_info->msg_in_delivering++;
        node_info->msg_in_publishing--;
    }
    up_write(&node_info->publishing_semaphore);
    up_write(&node_info->delivering_semaphore);
    return 0;
}

// read syscall()
ssize_t gmm_read(struct file * file, char * buffer, size_t lenght, loff_t * offset){
    struct message_queue * iter;
    ktime_t current_time;
    int list_is_empty;
    char * err_msg;
    int ret;
    node_information * node_info;
    node_info = file->private_data;
    if(lenght>bytes_per_message){
        return -ENOMEM;
    }
    if ( is_in_sleeping_tids(current->pid, node_info) ){
        return -EACCES;
    }
    current_time = ktime_get_boottime();
    iter =NULL;
    list_is_empty = list_empty(&node_info->publishing_queue.list);
    // If I have msgs in publishing queue, I try to post a valid message into the delivering queue
    if( !list_is_empty ){
        iter = get_minimum_message(node_info);
        if(iter==NULL){
            // I have no valid messages, so it's like having an empty queue
            goto next;
        }
        down_write(&node_info->publishing_semaphore);
        down_write(&node_info->delivering_semaphore);
        list_del(&iter->list);
        list_add_tail(&iter->list, &node_info->delivering_queue.list);
        node_info->msg_in_publishing--;
        node_info->msg_in_delivering++;
        up_write(&node_info->publishing_semaphore);
        up_write(&node_info->delivering_semaphore);
    }
    // I deliver the next message to the user
next:
    list_is_empty = list_empty(&node_info->delivering_queue.list);
    // read_unlock_irqrestore(&node_info->lock, node_info->lock_flags);
    if( !list_is_empty ){
        // write_lock_irqsave(&node_info->lock,node_info->lock_flags);
        down_write(&node_info->delivering_semaphore);
        iter = list_first_entry(&node_info->delivering_queue.list, struct message_queue, list);
        ret = strlen(iter->message->text);
        copy_to_user(buffer, iter->message->text, ret);
        list_del(&iter->list);
        vfree(iter->message->text);
        kfree(iter->message);
        node_info -> msg_in_delivering--;
        up_write(&node_info->delivering_semaphore);
        goto end;
    }
    else {
        // queue is empty
        ret = strlen(QUEUE_EMPTY_MESSAGE);
        err_msg = kmalloc(sizeof(char)*ret, GFP_KERNEL);
        if(err_msg==NULL){
            return -ENOMEM;
        }
        sprintf(err_msg, "%s", QUEUE_EMPTY_MESSAGE);
        copy_to_user(buffer, err_msg, strlen(err_msg));
        ret = 0;
        kfree(err_msg);
    }
    end:
        return ret;
}

ssize_t gmm_write(struct file * file, const char __user * buffer, size_t length, loff_t * offset ){
    int bytes_used;
    struct message_queue * current_message;
    node_information* node_info;
    ktime_t current_time, sending_time;
    node_info = file->private_data;
    bytes_used = (node_info->msg_in_delivering + node_info->msg_in_publishing)*bytes_per_message;
    // Checks on msg size and total size for the queue
    if(length > bytes_per_message || (bytes_used + bytes_per_message )>=total_bytes_in_queue){
        return -ENOMEM;
    }
    if( is_in_sleeping_tids(current->pid, node_info) ){
        return -EACCES;
    }
    if( (current_message = kmalloc(sizeof(struct message_queue), GFP_KERNEL)) == NULL ){
        return -ENOMEM;
    }
    if( ( current_message->message = kmalloc(sizeof(thread_message), GFP_KERNEL) ) == NULL ){
        return -ENOMEM;
    }
    if( ( current_message->message->text = kmalloc(sizeof(char)*bytes_per_message, GFP_KERNEL) ) == NULL ){
        return -ENOMEM;
    }
    if( node_info->sending_delay <=0 ){
        // direct publish in delivering queue
        // since delay is 0 or less than 0.
        copy_from_user(current_message->message->text, buffer, length);
        current_message->message->sender = current->pid;
        down_write(&node_info->delivering_semaphore);
        list_add_tail( &current_message->list, &node_info->delivering_queue.list );
        node_info->msg_in_delivering++;
        up_write(&node_info->delivering_semaphore);
        return strlen(current_message->message->text);
    }
    else {
        // I put the msg into the publishing queue
        current_time = ktime_get_boottime();
        sending_time = current_time + node_info -> sending_delay;
        current_message -> message->sender = current->pid;
        copy_from_user(current_message->message->text, buffer, length);
        current_message->publishing_time = sending_time;
        down_write(&node_info->publishing_semaphore);
        list_add_tail( &(current_message->list), &(node_info->publishing_queue.list) );
        node_info -> msg_in_publishing ++;
        up_write(&node_info->publishing_semaphore);
        return length;
    }
}


struct file_operations file_ops_gmm_origin = {
	.open= gmm_open,
    .read= gmm_read,
    .write= gmm_write,
    .release= gmm_release,
    .unlocked_ioctl= gmm_ioctl,
	.compat_ioctl= gmm_ioctl,
    .flush = gmm_flush,
	// unlocked_ioctl: mydev_ioctl,
	// compat_ioctl: mydev_ioctl,
	// release: mydev_release
};

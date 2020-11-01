#include "../thread_manager_spowner/thread_manager_spowner.h"
// #include "../group_message_manager/group_message_manager.h"


// this functions tries to open a group, if it exists it returns 
// a File Descriptor of the file we are operating to.
// If the group does not exists, the function tries to install it.
// if the operation fails, a number < 0 is returned.
int open_group(groupt * group_descriptor);

// tries to write a message into the group managed by the given file descriptor
int write_message(int file_descriptor, char * message, int max_length);

// read a message(if any) from the group managed by the given file descriptor
int read_message(int file_descriptor, char * buffer, int max_length);

// sets the publishing delay for the messages in the group described by the file descriptor
long set_message_delay(int file_descriptor, long new_delay);

// sets the thread having tid as gettid() in sleeping mode on the group managed by the device
// describer by the file descriptor
int sleep_tid(int file_descriptor, int tid);

// As the previous func, but in reverse
int awake_tids(int file_descriptor);

// int flush_group_messages(int file_descriptor);

void close_group(int file_descriptor);
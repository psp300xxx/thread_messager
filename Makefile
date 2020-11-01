# ioctl-objs := ./ioctl.o .
obj-m += thread_manager_spowner.o

obj-m += thread_manager_spowner.o
thread_manager_spowner-objs := ./src/thread_manager_spowner/thread_manager_spowner.o ./src/thread_manager_spowner/ioctl_switch_functions.o ./src/group_message_manager/group_message_manager.o


CURRENT_PATH = $(shell pwd)
LINUX_KERNEL = $(shell uname -r)
LINUX_KERNEL_PATH = /lib/modules/$(shell uname -r)/build/
all:
			make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
			gcc $(CURRENT_PATH)/src/user_space_test_app/single_thread_fifo_test.c $(CURRENT_PATH)/src/userspace_library/thread_msn.c -o $(CURRENT_PATH)/tests/single_thread_fifo_test.out
			gcc $(CURRENT_PATH)/src/user_space_test_app/multiple_threads.c $(CURRENT_PATH)/src/userspace_library/thread_msn.c -o $(CURRENT_PATH)/tests/multiple_threads.out -lpthread
			gcc $(CURRENT_PATH)/src/user_space_test_app/multiple_thread_multiple_group.c $(CURRENT_PATH)/src/userspace_library/thread_msn.c -o $(CURRENT_PATH)/tests/multiple_thread_multiple_group.out -lpthread
			gcc $(CURRENT_PATH)/src/user_space_test_app/delay_test.c $(CURRENT_PATH)/src/userspace_library/thread_msn.c -o $(CURRENT_PATH)/tests/delay_test.out
			gcc $(CURRENT_PATH)/src/user_space_test_app/sleep_thread_test.c $(CURRENT_PATH)/src/userspace_library/thread_msn.c -o $(CURRENT_PATH)/tests/sleep_thread_test.out



clean:
			make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
			rm  $(CURRENT_PATH)/tests/*


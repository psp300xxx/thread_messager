#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include "../userspace_library/thread_msn.h"


const int max_array_elements = 1000;

#define MAX_STRING_lEN 5

// current string val
char num_string [MAX_STRING_lEN];


int main(void){
    // set up data for tests
    srand(getpid());
    groupt * group_descriptor;
    int * array;
    int number_of_elements;
    int current_int;
    int ret;
    group_descriptor = malloc(sizeof(groupt));
    if(group_descriptor==NULL){
        perror("No memory!");
        return -1;
    }
    number_of_elements = rand()%max_array_elements;
    array = malloc(sizeof(int)*(number_of_elements));
    if(array==NULL){
        perror("No memory!");
        return -1;
    }
    for(int i = 0; i< number_of_elements; i++){
        array[i] = i;
    }
    group_descriptor->group = rand()%(max_array_elements/10);
    int fd = open_group(group_descriptor);
    printf("FD is %d\n", fd);
    if(fd<0){
        perror("Group not opened");
    }
    // write strings to the group
    for(int i = 0 ; i<number_of_elements; i++){
        sprintf(num_string, "%dk", array[i]);
        printf("Writing %s\n", num_string);
        ret = write_message(fd, num_string,MAX_STRING_lEN);
        if(ret<0){
            return -1;
        }
    }
    int NUMBER_PREDICTED_VALUES = 0;
    // I read again the messages
    // If it is fifo, I should have at the end NUMBER_PREDICTED_VALUES == number_of_elements
    printf("Before reading\n ");
    for(int i = 0 ; i<number_of_elements; i++){
        int ret = read_message(fd, num_string, MAX_STRING_lEN);
        if(ret<0){
            break;
        }
        current_int =  atoi(num_string);
        printf("prediction %d , real %d, string %s \n", i, current_int, num_string);
        if(current_int==i){
            NUMBER_PREDICTED_VALUES++;
        }
    }
    printf("Predicted %d, Elements %d\n", NUMBER_PREDICTED_VALUES, number_of_elements);
    close_group(fd);
    free(array);
    free(group_descriptor);
}
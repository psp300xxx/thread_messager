#include <stdio.h>
#include <stdlib.h>
#include "../userspace_library/thread_msn.h"
#include <pthread.h>

const long seconds_delay = 0.2;
int launch_group(groupt * group);

groupt * group_descriptors;
const int NUMBER_OF_GROUPS=10;
const int NUMBER_OF_THREAD_PER_GROUP =30;
void * read_and_write(groupt * group);
#define MAX_STRING_LEN 30
const int number_of_elements = 40;

int main(void){
    group_descriptors = malloc( sizeof(groupt)*NUMBER_OF_GROUPS );
    if(group_descriptors==NULL){
        return -1;
    }
    for(int i = 0 ; i<NUMBER_OF_GROUPS; i++){
        group_descriptors[i].group = i+1;
        group_descriptors[i].open_times = 0;
        launch_group(&group_descriptors[i]);
    }
}

int launch_group(groupt * group){
    pthread_t * thread_id;
    thread_id = malloc(sizeof(thread_id)*NUMBER_OF_THREAD_PER_GROUP);
    int start_id = rand()%100 + (2000);
    for(int i = 0; i<NUMBER_OF_THREAD_PER_GROUP; i++){
        thread_id[i] = start_id++;
        if(pthread_create(&thread_id[i], NULL, read_and_write, group)) {
            fprintf(stderr, "Error creating thread %d \n", start_id);
        }
    }
}


void * read_and_write(groupt * group_desc){
    char ** array;
    int fd;
    int ret;
    char * message;
    fd = open_group(group_desc);
    if(fd<0){
        perror("opening group");
        group_desc ->open_times ++;
        return NULL;
    }
    message = malloc(sizeof(char)*MAX_STRING_LEN);
    if(message==NULL){
        perror("memory");
        group_desc ->open_times ++;
        return NULL;
    }
    array = malloc(sizeof(char *)*number_of_elements);
    if(array==NULL){
        perror("memory");
        group_desc ->open_times ++;
        free(message);
        return NULL;
    }
    for(int i =0; i<number_of_elements;i++){
        array[i] = malloc(sizeof(char)*MAX_STRING_LEN);
        if(array[i]==NULL){
            perror("memory");
            group_desc ->open_times ++;
            return NULL;
        }
        sprintf(array[i], "%d\n", group_desc->group,gettid(), i);
    }
    printf("fd is %d\n", fd);
    for(int i = 0 ; i<number_of_elements; i++){
        ret = write_message(fd, array[i], strlen(array[i]));
        if(ret<0){
            perror("writing");
            break;
        }
        sleep(seconds_delay);
    }  
    for(int i = 0 ; i<number_of_elements; i++){
        sleep(seconds_delay);
        ret = read_message(fd, message, MAX_STRING_LEN);
        if(ret<0){
            perror("reading");
            break;
        }
        int group_id;
        printf("\n%s\n\n", message);
        sscanf(message,"%d",&group_id);
        if( group_desc->group != group_id ){
            fprintf(stderr, "Error: WRONG GROUP group_id  %d, real_group %d\n", group_id, group_desc->group);
            break;
        }
    }
    close_group(fd);
    // free dataf
    group_desc ->open_times ++;
    free(message);
    for(int i =0 ; i<number_of_elements; i++){
        free(array[i]);
    }
    free(array);  
}
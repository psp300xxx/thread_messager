// This is an header file common to all the project..

typedef struct {
    int group;
    int open_times;
} groupt;

extern int bytes_per_message;

extern int total_bytes_in_queue;

typedef struct {
    char * text;
    int sender;
} thread_message;

// IOCTL INFORMATIONS

#define GMM_IOC_MAGIC 'R'

#define IOCTL_GMM_SET_DELAY _IOW(GMM_IOC_MAGIC, 1, long )

#define IOCTL_GMM_SLEEP_TID _IOW(GMM_IOC_MAGIC, 2, long )

#define IOCTL_GMM_AWAKE_TIDS _IO(GMM_IOC_MAGIC,3 )

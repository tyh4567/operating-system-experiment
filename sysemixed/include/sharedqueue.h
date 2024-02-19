

#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct shared_queue_class
{
    int this_id; 
    int next_id; 
	int hit, fd; 
}shared_queue_class;

typedef struct shared_queue_ref
{
    int len; 
    int head; 
    int rear; 
    int num_queue; 
}shared_queue_ref;

typedef union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
}semun_def;

#define KEY_FOR_SHARED_QUEUE_MEMORY 1306
#define KEY_FOR_SHARED_QUEUE_MEMORY_REF 1057
#define KEY_FOR_SHARED_QUEUE_SIGNAL 1158
#define KEY_FOR_SHARED_QUEUE_SIGNAL_REF 1199

enum{
    LEN, HEAD, REAR, NUMQUEUE
};


int init_shared_queue();
int push_task_to_shared_queue(shared_queue_class* sqc);
int take_task_from_shared_queue(shared_queue_class* sqc);
int get_shared_queue_len();
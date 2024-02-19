#include <sys/sem.h>

#define sem_key 0x12407 
typedef union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
}semun_def;

void sem_init();

int sem_signal();

int sem_wait();

void sem_del();
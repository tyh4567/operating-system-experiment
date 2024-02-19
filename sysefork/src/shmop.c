#include "../include/shmop.h"

int shmid;
void shm_init()
{
    shmid = shmget(shm_key, sizeof(double), IPC_CREAT | IPC_EXCL);
    if(shmid < 0) 
    {
        perror("shm_init");
    }
    void* addr = shmat(shmid, 0, 0);
    *(double*)addr = 0;
    shmdt(addr);
}

void shm_send_time(double ti)
{
    void* addr = shmat(shmid, 0, 0);
    double ls = *(double*)addr;
    ls += ti;
    *(double*)addr = ls;
    // printf("all time is %lf\n", *(double*)addr);
    shmdt(addr);
}

double shm_read()
{
    double num;
    void* addr = shmat(shmid, 0, 0);
    num = *(double*)addr;

    shmdt(addr);
    return num;
}

void shm_del()
{
    if(shmctl(shmid, IPC_RMID, 0) == -1)
    {
        perror("shum_del");
    }
}
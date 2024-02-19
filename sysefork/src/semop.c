#include "../include/semop.h"

int semid;


void sem_init()
{
	semid = semget(sem_key,1, IPC_CREAT);
    if(semid < 0)
    {             
          perror("semget");            
          return 0;   
    }
    semun_def sem_union;    
    sem_union.val = 1;
    if(semctl(semid, 0, SETVAL, sem_union)==-1)
    {
        perror("seminit");     
        return 0;
    }
}

int sem_signal()
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(semid,&sem_b,1)==-1)
    {
        perror("signael");
        return 0;
    }
    return 1;
}


int sem_wait()
{
    struct sembuf sem_b;
	
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(semid,&sem_b,1)==-1)
    {
        perror("wait");
        return 0;
    }
    return 1;
}

void sem_del()
{
    union semun sem_union;
    if(semctl(semid,0,IPC_RMID,sem_union)==-1)
        perror("del");
}

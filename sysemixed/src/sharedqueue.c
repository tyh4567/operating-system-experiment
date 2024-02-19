#include "../include/sharedqueue.h"

int sem_signal();
int sem_wait();

int queue_id; 
int sem_id; 
int sem_id_ref; 
int shm_id_ref; 

void init_ref()
{
    while(1)
    {
        if(sem_wait(sem_id_ref)) break; 
    }
    int shmid = shmget(KEY_FOR_SHARED_QUEUE_MEMORY_REF, sizeof(shared_queue_ref), IPC_CREAT|0666);
    if(shmid < 0)
    {
        perror("error to get shared memory\n"); 
        exit(-1); 
    }
    shm_id_ref = shmid; 
    shared_queue_ref* addr = (shared_queue_ref*)shmat(shmid, NULL, 0); 
    addr->head = 0; 
    addr->rear = 0; 
    addr->len = 0; 
    addr->num_queue = 0; 
    sem_signal(sem_id_ref); 
}

void change_ref(int i, int num) //if -1 don't change
{
    while(1)
    {
        if(sem_wait(sem_id_ref)) break; 
    }
    
    shared_queue_ref* addr = (shared_queue_ref*)shmat(shm_id_ref, NULL, 0); 
    if(i == LEN) addr->len = num;
    else if(i == HEAD) addr->head = num;
    else if(i == REAR) addr->rear = num;
    else if(i == NUMQUEUE) addr->num_queue = num;
    // if(i == LEN) printf("len has changed, and its value == %d\n", num); 
    shmdt(addr); 

    sem_signal(sem_id_ref); 
}

int get_ref(int i) //0 len 1 head 2 rear 3 num_queue
{
    int ls; 

    while(1)
    {
        if(sem_wait(sem_id_ref)) break; 
    }
    
    shared_queue_ref* addr = (shared_queue_ref*)shmat(shm_id_ref, NULL, 0);
    if(i == LEN) ls = addr->len; 
    if(i == HEAD) ls = addr->head; 
    if(i == REAR) ls = addr->rear; 
    if(i == NUMQUEUE) ls = addr->num_queue; 
    shmdt(addr); 

    sem_signal(sem_id_ref);  
    return ls; 
}

int init_shared_queue()
{

    sem_id = semget(KEY_FOR_SHARED_QUEUE_SIGNAL, 1, IPC_CREAT);
    sem_id_ref = semget(KEY_FOR_SHARED_QUEUE_SIGNAL_REF, 1, IPC_CREAT);

    semun_def sem_union;    
    sem_union.val = 1;

    if(sem_id < 0)
    {             
        perror("semget");            
        return 0;   
    }
    if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
    {
        perror("seminit");     
        return 0;
    }

    if(sem_id_ref < 0)
    {             
        perror("semget");            
        return 0;   
    }
    if(semctl(sem_id_ref, 0, SETVAL, sem_union) == -1)
    {
        perror("seminit");     
        return 0;
    }

    init_ref(); 
    return 1; 
}

int push_task_to_shared_queue(shared_queue_class* sqc)
{
    while(1)
	{
		if(sem_wait(sem_id)) break;
	}

    if(sqc == NULL)
    {
        perror("error in push_task_to_shared_queue, and the sqc is null\n"); 
        exit(-1); 
    }

    int shmid = shmget(KEY_FOR_SHARED_QUEUE_MEMORY + get_ref(NUMQUEUE), sizeof(shared_queue_class), IPC_CREAT|0666);
    if(shmid < 0)
    {
        perror("error to get shared memory\n"); 
        exit(-1); 
    }
    change_ref(NUMQUEUE, get_ref(NUMQUEUE) + 10);
    shared_queue_class* addr = (shared_queue_class*)shmat(shmid, NULL, 0); 
    if(addr == NULL)
    {
        perror("error in get addr\n"); 
        exit(-1);
    }
    
    addr->fd = sqc->fd; 
    addr->hit = sqc->hit; 
    addr->this_id = shmid; 
    addr->next_id = 0; 
    shmdt(addr); 

    change_ref(LEN, get_ref(LEN) + 1);
    // printf("# succeed to send data to shared queue, and the len is %d\n", get_ref(LEN));

    if(get_ref(LEN) == 1)
    {
        change_ref(HEAD, shmid); 
        change_ref(REAR, shmid); 
    }
    else 
    {
        addr = (shared_queue_class*)shmat(get_ref(REAR), NULL, 0);
        if(addr == NULL)
        {
            perror("error in push_task_to_shared_queue, and the line is 94\n"); 
            exit(-1);
        }
        addr->next_id = shmid; 
        shmdt(addr); 
        change_ref(REAR, shmid); 
    }
    
    sem_signal(sem_id); 
    return 1; 
}

int take_task_from_shared_queue(shared_queue_class* sqc)
{
    while(1)
	{
		if(sem_wait(sem_id)) break;
	}
    if(get_ref(LEN) <= 0) 
    {
        sem_signal(sem_id); 
        return 0; 
    }
    if(get_ref(HEAD) == 0)
    {
        perror("error in take_task_from_shared_queue, because the queue_head is null\n");
        return 0; 
    }
    shared_queue_class* addr = (shared_queue_class*)shmat(get_ref(HEAD), NULL, 0); 
    sqc->fd = addr->fd; 
    sqc->hit = addr->hit; 
    sqc->next_id = addr->next_id; 
    sqc->this_id = addr->this_id;
    shmdt(addr);
    change_ref(LEN, get_ref(LEN) - 1); 
    printf("# succeed to get data from shared queue, and the fd is %d\n", sqc->fd);

    int shmid = get_ref(HEAD); 
    shmctl(shmid, IPC_RMID, NULL); 
    change_ref(HEAD, sqc->next_id);
    printf("# succeed to release data from shared queue, and the fd is %d\n", sqc->fd);
    sem_signal(sem_id);
    return 1; 
}

int get_shared_queue_len()
{
    return get_ref(LEN); 
}

int sem_signal(int semid)
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

int sem_wait(int semid)
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
#include <stdio.h> 
#include <stdbool.h>
#include <sys/prctl.h>
#include <pthread.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <fcntl.h> 
#include <signal.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/stat.h> 
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/signal.h>

#define TASKQUEUE_MAX 10

typedef struct staconv
{
    pthread_mutex_t mutex; 
    pthread_cond_t cond; 
    int status; 
}staconv; 

//task
typedef struct task
{
    struct task* next; 
    void (*function)(void* arg);  //?
    void* arg; 
} task; 

//task queue
typedef struct taskqueue
{
    pthread_mutex_t mutex; 
    task* front; 
    task* rear; 
    staconv *has_jobs; 
    int len; 
}taskqueue; 

//thread
typedef struct thread{
    int id; 
    pthread_t pthread; 
    struct threadpool* pool; 
}thread; 

typedef struct threadpool
{
    thread** threads; 
    volatile int num_threads; 
    volatile int num_working; 
    pthread_mutex_t thcount_lock; 
    pthread_cond_t threads_all_idle; 
    taskqueue queue; 
    volatile bool is_alive; 
}threadpool; 

struct threadpool* initThreadPool(int num_threads);
void addTask2ThreadPool(threadpool* pool, task* curtask);
void destoryThreadPool(threadpool* pool);
int getNumofThreadWorking(threadpool* pool);

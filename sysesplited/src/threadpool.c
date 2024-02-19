#include "../include/threadpool.h"
#include <sys/time.h>
void init_taskqueue(taskqueue* tq);
int create_thread(struct threadpool* pool, struct thread** pthread, int id);
void push_taskqueue(taskqueue* tq, task* t);

int time_all_run = 0; 
int time_all = 0; 
int time_init = 0; 
int thread_all = 0; 
int sum_pool = 0;
pthread_mutex_t time_mutex; 

void add_all_time(int times)
{
    pthread_mutex_lock(&(time_mutex)); 
    time_all_run += times; 
    time_all = time(NULL); 
    thread_all++; 
    pthread_mutex_unlock(&(time_mutex));
}

int get_time_all()
{
    return time_all - time_init; 
}

int get_time_all_run()
{
    return time_all_run; 
}

int get_thread_all(int id_pool)
{
    return thread_all; 
}

struct threadpool* initThreadPool(int num_threads)
{
    threadpool* pool;
    pool = (threadpool*)malloc(sizeof(struct threadpool)); 
    pool->is_alive = true; 
    pool->num_threads = 0; 
    pool->num_working = 0; 

    time_init = time(NULL); 
    //初始化互斥量和条件变量
    pthread_mutex_init(&(pool->thcount_lock), NULL); 
    pthread_cond_init(&(pool->threads_all_idle), NULL); 
    pthread_mutex_init(&(time_mutex), NULL); 
    //初始化任务队列

    init_taskqueue(&(pool->queue));

    //创建线程数组
    pool->threads = (struct thread**)malloc(num_threads*sizeof(struct thread*));

    //创建线程
    for(int i = 0; i < num_threads; ++i)
    {
        create_thread(pool, &(pool->threads[i]), i); 
    }

    while(pool->num_threads != num_threads){
        // //printf("%d\n", pool->num_threads);
    }
    // *id_pool = sum_pool; 
    // sum_pool++;

    return pool; 
}

void init_taskqueue(taskqueue* tq)
{
    taskqueue* this = (taskqueue*)malloc(sizeof(taskqueue)); 
    pthread_mutex_init(&(this->mutex), NULL); 
    this->front = NULL;
    this->rear = NULL;
    this->has_jobs = (staconv*)malloc(sizeof(staconv)); 
    this->has_jobs->status = 0; 
    pthread_mutex_init(&(this->has_jobs->mutex), NULL); 
    pthread_cond_init(&(this->has_jobs->cond), NULL);
    this->len = 0; 
    *tq = *this; 
}

void destory_taskqueue(taskqueue* tq)
{
    task* task_ready_to_destory = tq->front; 
    while(task_ready_to_destory != NULL)
    {
        tq->front = tq->front->next; 
        //free(task_ready_to_destory->arg);  //?
        free(task_ready_to_destory);
        task_ready_to_destory = tq->front; 
    }
    free(tq->has_jobs);
    free(tq); 
}

task* take_taskqueue(taskqueue* tq)
{
    //printf("tqmetex2\n");
    // pthread_mutex_lock(&(tq->mutex)); 
    //printf("tqmetex2get\n");
    // tq->len -= 1; 
    task* t = tq->front; 
    if(tq->front == NULL)
    {
        printf("this %d\n", tq->len);
        perror("error 13\n"); 
        exit(-1);
    }
    // tq->len--; 
    //printf("th%d\n", tq->len); 
    // if(tq->len == 0)
    // {
    //     pthread_mutex_lock(&(tq->has_jobs->mutex));
    //     tq->has_jobs->status = 0; 
    //     pthread_mutex_unlock(&(tq->has_jobs->mutex));
    //     //pthread_cond_signal(&(tq->has_jobs->cond)); //?????????????
    // }
    tq->front = tq->front->next; 
    // pthread_mutex_unlock(&(tq->mutex)); 
    //printf("tqmetex2push\n");
    return t; 
}

void push_taskqueue(taskqueue* tq, task* t)
{
    int should_to_change_status = 0; 
    //printf("tqmetex1\n");
    pthread_mutex_lock(&(tq->mutex)); 
    //printf("tqmetex1get\n");

    tq->len += 1; 
    if(tq->len == 1)
    {
        should_to_change_status = 1;
        tq->front = t;
        tq->rear = t;
        tq->front->next = NULL;
        // tq->has_jobs->status = true; 
    }
    else if(tq->len < 1)
    {
        perror("push_task\n"); 
        return;
    }
    else
    {
        tq->rear->next = t; 
        tq->rear = t; 
    }

    pthread_mutex_unlock(&(tq->mutex));
    //printf("job2\n");
    pthread_mutex_lock(&(tq->has_jobs->mutex)); 
    //printf("job2get\n");
    tq->has_jobs->status = true; 
    pthread_mutex_unlock(&(tq->has_jobs->mutex)); 
    //printf("job2push\n");
    //printf("tqmetex1push\n");

    // if(should_to_change_status == 1)
    // {
        pthread_cond_signal(&(tq->has_jobs->cond)); 
    // }
    
}

void addTask2ThreadPool(threadpool* pool, task* curtask)
{
    //printf("get one port\n");
    push_taskqueue(&(pool->queue), curtask); 
}

//等待所有线程释放完毕
void waitThreadPool(threadpool* pool)
{
    pthread_mutex_lock(&(pool->thcount_lock)); 
    while(pool->queue.len || pool->num_working)
    {
        pthread_cond_wait(&(pool->threads_all_idle), &(pool->thcount_lock)); 
    }
    pthread_mutex_unlock(&(pool->thcount_lock));
}

//销毁线程池
void destoryThreadPool(threadpool* pool)
{
    pthread_mutex_lock(&(pool->queue.has_jobs->mutex)); 
    while(pool->queue.has_jobs->status == true)
    {
        pthread_cond_wait(&(pool->queue.has_jobs->cond), &(pool->queue.has_jobs->mutex)); 
    }
    pthread_mutex_unlock(&(pool->queue.has_jobs->mutex)); 
    pool->is_alive = false; 
    waitThreadPool(pool); 
    destory_taskqueue(&(pool->queue)); 
    free(pool->threads);
    free(pool);  
}

//获取正在运行的线程数量
int getNumofThreadWorking(threadpool* pool)
{
    return pool->num_working; 
}

int getNUMofTaskQueue(threadpool* pool)
{
    return pool->queue.len; 
}

//has_jobs??89
void* thread_do(struct thread* pthread)
{
    char thread_name[128] = {0}; 
    sprintf(thread_name, "thread-pool-%d", pthread->id); 
    prctl(PR_SET_NAME, thread_name); 
    threadpool* pool = pthread->pool; 
    //更新线程池
    pthread_mutex_lock(&(pool->thcount_lock)); 
    (pool->num_threads)++; 
    pthread_mutex_unlock(&(pool->thcount_lock)); 

    //运行
    while(pool->is_alive)
    {
        //printf("job1\n");
        
        pthread_mutex_lock(&(pool->queue.has_jobs->mutex));
        //printf("job1get\n");
        // printf("ready thread is == %s\n", thread_name); 
        while(pool->queue.has_jobs->status == 0 || pool->queue.len == 0)
        {
            //printf("job1push\n");
            pthread_cond_wait(&(pool->queue.has_jobs->cond), &(pool->queue.has_jobs->mutex)); 
            //printf("job1get2\n");
        }

        //printf("tqmetex3\n");
        pthread_mutex_lock(&(pool->queue.mutex)); 
        //printf("tqmetex3get\n");
        // printf("queue len == %d\n", pool->queue.len); 
        int time_start = time(NULL); 
        struct timeval start, end;
        gettimeofday( &start, NULL );

        pool->queue.len -= 1;
        task* curtask;
        curtask = take_taskqueue(&(pool->queue)); 
        //printf("this len is == %d\n", pool->queue.len); 
        if(pool->queue.len == 0)
        {
            pool->queue.has_jobs->status = 0; 
        }
        else if(pool->queue.len < 0)
        {
            perror("error 11\n"); 
        }

        pthread_mutex_unlock(&(pool->queue.mutex)); 
        //printf("tqmetex3push\n");

        pthread_mutex_unlock(&(pool->queue.has_jobs->mutex));
        //printf("job1push2\n");
        if(pool->is_alive)
        {
            //printf("tqmetex4\n");
            pthread_mutex_lock(&(pool->thcount_lock)); 
            //printf("tqmetex4get\n");
            pool->num_working++; 
            // printf("this thread is == %s\n", thread_name); 
            pthread_mutex_unlock(&(pool->thcount_lock)); 
            //printf("tqmetex4push\n");
            //提取任务
            void (*func)(void*); 
            void* arg; 
            // task* curtask = take_taskqueue(&(pool->queue)); 
            //printf("succeed to get task\n");
            if(curtask)
            {
                func = curtask->function; 
                arg = curtask->arg; 
                func(arg); 
            }
            int time_end = time(NULL); 
            int time_run = time_end - time_start; 


            gettimeofday( &end, NULL );
            int timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 
            // add_all_time(timeuse, ); 
            //改变工作线程数量
            //printf("tqmetex5\n");
            pthread_mutex_lock(&(pool->thcount_lock)); 
            //printf("tqmetex5get\n");
            pool->num_working--; 
            
            if(pool->num_working == 0)
            {
                pthread_cond_signal(&(pool->threads_all_idle)); 
            }
            else if(pool->num_working < 0)
            {
                perror("error 12\n"); 
            }
            pthread_mutex_unlock(&(pool->thcount_lock)); 
            // //printf("tqmetex5push\n");
        }
    }



    //线程退出??
    pthread_mutex_lock(&(pool->thcount_lock)); 
    pool->num_threads--; 
    pthread_mutex_unlock(&(pool->thcount_lock)); 

    return NULL; 
}

int create_thread(struct threadpool* pool, struct thread** pthread, int id)
{
    *pthread = (struct thread*)malloc(sizeof(struct thread)); 
    if(pthread == NULL)
    {
        perror("create_thread\n"); 
        return -1; 
    }
    //设置属性
    (*pthread)->pool = pool; 
    (*pthread)->id = id; 
    pthread_create(&((*pthread)->pthread), NULL, (void*)thread_do, (*pthread)); 
    pthread_detach((*pthread)->pthread); 
    return 0; 

}

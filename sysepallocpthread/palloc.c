#include "palloc.h"

struct timeval start1, end1;
struct timeval start2, end2;
struct timeval start3, end3;
int time1 = 0; 
int time2 = 0; 
int time3 = 0; 
// struct timeval start, end;
// gettimeofday(&start, NULL);
// gettimeofday(&end, NULL);
// time1 += 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 

palloc_large* large_rear[MAX_NUM_OF_POOL]; 
pthread_mutex_t palloc_mutex[MAX_NUM_OF_POOL]; //锁
palloc_pool* pool_init[MAX_NUM_OF_POOL];
palloc_pool* pool_now[MAX_NUM_OF_POOL];
int pool_index = 0; 

//返回值为pool索引
int init_palloc_pool()
{
    pthread_mutex_init(&(palloc_mutex[pool_index]), NULL);
    palloc_pool* new_pool = (palloc_pool*)aligned_alloc(PALLOC_ALIGNMENT, SIZE_OF_POOL);
    new_pool->control.start = palloc_align_ptr((char*)new_pool + sizeof(palloc_pool), PALLOC_ALIGNMENT);
    new_pool->control.end = (char*)new_pool + SIZE_OF_POOL;
    if((new_pool->control.end - new_pool->control.start) < MAX_SIZE)
    {
        printf("max_size is lager than size_of_pool\n");
        exit(-1);
    }
    new_pool->control.next = NULL;
    new_pool->control.large_size = 0; 
    new_pool->large = NULL;
    pool_init[pool_index] = new_pool;
    pool_now[pool_index] = new_pool; 
    pool_index++;
    return pool_index - 1; 
}

palloc_pool* create_palloc_pool()
{
    palloc_pool* new_pool = (palloc_pool*)aligned_alloc(PALLOC_ALIGNMENT, SIZE_OF_POOL);
    new_pool->control.start = palloc_align_ptr((char*)new_pool + sizeof(palloc_pool), PALLOC_ALIGNMENT);
    new_pool->control.end = (char*)new_pool + SIZE_OF_POOL;
    new_pool->control.next = NULL;
    new_pool->control.large_size = 0; 
    new_pool->large = NULL;
    return new_pool;
}

palloc_large* create_palloc_large(long long size)
{
    palloc_large* p = (palloc_large*)aligned_alloc(PALLOC_ALIGNMENT, size + sizeof(palloc_large) + PALLOC_ALIGNMENT);
    p->next = NULL;
    p->start = palloc_align_ptr((char*)p + sizeof(palloc_large), PALLOC_ALIGNMENT);
    return p; 
}

void* palloc(long long size, int index)
{
    palloc_pool* iter = pool_init[index]; 
    if(PTHREAD_SUPPORT) pthread_mutex_lock(&(palloc_mutex[index]));
    if(size <= MAX_SIZE)
    {
        //改进，参考文档思路3
        // palloc_pool* iter_before = pool_init; 
        // while(iter != NULL)
        // {
        //     // gettimeofday(&start2, NULL);
        //     char* p = iter->control.start;
        //     p = palloc_align_ptr(p, PALLOC_ALIGNMENT);
        //     //printf("size = %d, this = %d\n", size, iter->control.end-p);
        //     if((iter->control.end - p) >= size)
        //     {
        //         iter->control.start = p + size;
        //         // pthread_mutex_unlock(&(palloc_mutex));
        //         return p;
        //     }
        //     if(iter != pool_init) iter_before = iter; 
        //     iter = iter->control.next;
        //     // gettimeofday(&end2, NULL);
        //     // time2+= 1 * ( end2.tv_sec - start2.tv_sec ) + end2.tv_usec - start2.tv_usec; 
        // }
        // iter = iter_before;
        
        char* p = pool_now[index]->control.start;
        p = palloc_align_ptr(p, PALLOC_ALIGNMENT);

        if(((pool_now[index]->control.end) - p) >= size)
        {
            // 改进，参考文档思路3
            // iter->control.start = p + size;
            pool_now[index]->control.start = p + size; 
            if(PTHREAD_SUPPORT) pthread_mutex_unlock(&(palloc_mutex[index]));
            return p;
        }
        iter->control.next = create_palloc_pool();

        iter = iter->control.next;
        pool_now[index] = iter; 
        // char* p = iter->control.start;
        p = iter->control.start;
        p = palloc_align_ptr(p, PALLOC_ALIGNMENT);
        if(PTHREAD_SUPPORT) pthread_mutex_unlock(&(palloc_mutex[index]));

        return p; 
    }
    else
    {
        palloc_large* p = create_palloc_large(size); 
        palloc_large* init_large = pool_init[index]->large;
        if(init_large == NULL)
        {
            iter->large == p;
            large_rear[index] = p; 
        }
        else
        {
            large_rear[index]->next = p; 
            large_rear[index] = p; 
        }
        if(PTHREAD_SUPPORT) pthread_mutex_unlock(&(palloc_mutex[index]));
        return p->start;

        //改进，参考文档思路3
        // palloc_large* p = create_palloc_large(size); 
        // while(iter->control.large_size > MAX_LARGE_PER_POOL && iter->control.next != NULL)
        // {
        //     iter = iter->control.next;
        // }
        // palloc_large* iter_large = iter->large;
        // if(iter_large == NULL)
        // {
        //     iter->large == p;
        //     iter->control.large_size++;
        // }
        // else
        // {
        //     while(iter_large->next != NULL) iter_large = iter_large->next;
        //     iter->control.large_size++;
        //     iter_large->next = p; 
        // }
        // // pthread_mutex_unlock(&(palloc_mutex));
        // return p->start;
    }
}

int delete_palloc(int index)
{
    palloc_pool* iter = pool_init[index]; 
    while(iter != NULL)
    {
        palloc_large* iter_large = iter->large; 
        while(iter_large != NULL)
        {
            palloc_large* next_large = iter_large->next; 
            free(iter_large); 
            iter_large = next_large; 
        }
        palloc_pool* next_pool = iter->control.next; 
        free(iter); 
        iter = next_pool; 
    }
    return 1; 
}

// int main()
// {
//     int* a; 
//     struct timeval start, end;
//     gettimeofday(&start, NULL);
//     int id = init_palloc_pool(); 
//     for(int i = 0; i < 300000; i++)
//     {
//         a = palloc(1*sizeof(int), id);
//     }
//     delete_palloc(id);
//     gettimeofday(&end, NULL);
//     int timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 
//     printf("palloc_time == %d\n", timeuse);
    
//     gettimeofday(&start, NULL);
//     for(int i = 0; i < 300000; i++)
//     {
//         a = (int*)malloc(1*sizeof(int));
//         free(a);
//     }
//     gettimeofday(&end, NULL);
//     timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 
//     printf("malloc_time == %d\n", timeuse);

//     printf("time1 = %d\n", time1); 
//     printf("time2 = %d\n", time2); 
//     return 0; 
// }

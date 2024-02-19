#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#define PALLOC_ALIGNMENT 16 //按16字节对齐
#define MAX_LARGE_PER_POOL 5
#define MAX_NUM_OF_POOL 10
#define SIZE_OF_POOL 800000*sizeof(int)
#define MAX_SIZE 50000*sizeof(int)
#define PTHREAD_SUPPORT 0
//采用ngx的内存对齐
#define palloc_align_ptr(p, a)                                                   \
    (char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

typedef struct palloc_pool_control_unit //小内存池控制单元
{
    char* start; //开始分配
    char* end; //结束地址
    struct palloc_pool* next; //下一小内存池
    int large_size; 
}palloc_pool_control_unit;

typedef struct palloc_large //大内存
{
    char* start; 
    struct palloc_large* next;
}palloc_large;

typedef struct palloc_pool //小内存池
{
    struct palloc_pool_control_unit control;
    struct palloc_large* large; 
}palloc_pool;

int delete_palloc(int index);
void* palloc(long long size, int index);
int init_palloc_pool();
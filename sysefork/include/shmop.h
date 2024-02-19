#include <sys/shm.h>//共享内存

#define shm_key 0x12306  
void shm_init();

void shm_send_time(double ti);

double shm_read();

void shm_del();
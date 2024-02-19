#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_CACHE_LEN 500

typedef struct cache_content
{
    char* this_name;
    int length; 
    int times; 
    long long file_len; 
    char** cache_buf;
    struct cache_content* next; 
    struct cache_content* hash_before;
    struct cache_content* hash_next;
}cache_content;


void init_cache(int select_lru_);
int add_cache(char* name, int len, long long file_len, char** buf);
cache_content* search_cache(char* name);
int delete_cache();
int after_use(cache_content* c);
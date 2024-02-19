#include  "cache.h"

int select_lru = 0; 
int cache_len; 
cache_content* cache_head;
cache_content* cache_rear; 
cache_content* hash_t[10]; 

void init_cache(int select_lru_)
{
    if(select_lru_) select_lru = 1; 
    else select_lru = 0; 
    cache_head = NULL; 
    cache_rear = NULL; 
    cache_len = 0; 
}

int add_cache(char* name, int len, long long file_len, char** buf)
{
    if(cache_len > MAX_CACHE_LEN) delete_cache();
    cache_content* cc = (cache_content*)malloc(sizeof(cache_content));
    cc->this_name = name; 
    cc->file_len = file_len;
    cc->length = len;
    cc->cache_buf = buf;
    cc->next = cache_head; 
    cache_head = cc; 
    cache_len++;
    if(!select_lru) cc->times = 1; 
    cache_content* hash_p = hash_t[name[0] % 10];
    if(hash_p == NULL)
    {
        hash_t[name[0] % 10] = cc;
        cc->hash_before = NULL;
        cc->hash_next = NULL;
    }
    else
    {
        while(hash_p->hash_next != NULL) hash_p = hash_p->hash_next;
        cc->hash_before = hash_p; 
        cc->hash_next = NULL;
        hash_p->hash_next = cc;
    }
    return 1;
}

cache_content* search_cache(char* name)
{
    cache_content* hash_p = hash_t[name[0] % 10];
    if(hash_p == NULL) return NULL;
    while(strcmp(hash_p->this_name, name) != 0 && hash_p->hash_next != NULL) hash_p = hash_p->hash_next;
    if(strcmp(hash_p->this_name, name) == 0) return hash_p;
    return NULL;

    // cache_content* cc = cache_head; 
    // if(cc == NULL) return NULL;
    // while(cc->next != NULL)
    // {
    //     if(strcmp(cc->this_name, name) == 0) break; 
    //     cc = cc->next; 
    // }
    // if(strcmp(cc->this_name, name) == 0) 
    // {
    //     if(!select_lru) cc->times++; 
    //     return cc; 
    // }
    // return NULL;
}

int delete_cache()
{
    cache_content* cc = cache_head; 
    if(cc == NULL) return 1; 
    else if(cc == cache_rear)
    {
        if(cc->hash_before != NULL) cc->hash_before->hash_next = cc->hash_next;
        if(cc->hash_next != NULL) cc->hash_next->hash_before = cc->hash_before;
        for(int i = 0; i < cc->length; i++) free(cc->cache_buf[i]);
        free(cc->cache_buf);
        free(cc);
        cache_head = NULL; 
        cache_rear = NULL; 
        cache_len = 0; 
    }
    if(select_lru)
    {
        if(cache_rear->hash_before != NULL) cache_rear->hash_before->hash_next = cache_rear->hash_next;
        if(cache_rear->hash_next != NULL) cache_rear->hash_next->hash_before = cache_rear->hash_before;
        while(cc->next != cache_rear) cc = cc->next;
        for(int i = 0; i < cache_rear->length; i++) free(cache_rear->cache_buf[i]);
        free(cache_rear->cache_buf);
        free(cache_rear);
        cache_len--;
        cc->next = NULL;
        cache_rear = cc;
    }
    else
    {
        int min_times = cache_head->times; 
        cache_content* min_cc = cache_head; 
        cache_content* before_cc = cache_head;  
        cache_content* cc = cache_head; 
        while(cc->next != NULL)
        {
            if(cc->next->times < min_times)
            {
                min_times = cc->next->times; 
                min_cc = cc->next; 
                before_cc = cc;
            }
            cc = cc->next; 
        }
        if(min_cc->hash_before != NULL) min_cc->hash_before->hash_next = min_cc->hash_next;
        if(min_cc->hash_next != NULL) min_cc->hash_next->hash_before = min_cc->hash_before;
        before_cc->next = min_cc->next;
        for(int i = 0; i < min_cc->length; i++) free(min_cc->cache_buf[i]);
        free(min_cc->cache_buf);
        free(min_cc);
        cache_len--;
    }
    return 1; 
}

int after_use(cache_content* c)
{
    if(select_lru)
    {
        if(c == cache_head)
        {
            return 1; 
        }
        cache_content* cc = cache_head;
        while(cc->next != c) cc = cc->next;
        cc->next = c->next;
        c->next = cache_head;
        cache_head = c; 
    }
    else
    {
        c->times++;
    }
    return 1;
}

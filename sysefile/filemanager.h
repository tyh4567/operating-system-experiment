#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#define MB_LEN(x) (x * 1024 * 1024)
#define SIZE_OF_MEMORY 10725 //总大小10725MB
#define SIZE_OF_CONTROL 512 //control区大小
#define SIZE_OF_BLOCK 512 //一个块512字节
#define SIZE_OF_FILE_NAME 20 //文件名最大长度
#define SIZE_OF_DATA (SIZE_OF_BLOCK - 4)
#define NUM_OF_BLOCK (SIZE_OF_MEMORY*2048) //块总数
#define MAX_SIZE_OF_FILE MB_LEN(20) //文件最大20mb
#define MAX_NUM_OF_FILE 100000 //最大文件数
#define NUM_OF_BLOCK_BITMAP (NUM_OF_BLOCK/SIZE_OF_BLOCK+1) //块位图数
#define NUM_OF_INODE_BITMAP (MAX_NUM_OF_FILE/SIZE_OF_BLOCK+1) //inode位图数
#define NUM_OF_DIR_ENTRY_PER_BLOCK 19
#define NUM_OF_INIT_DIR_BLOCK (MAX_NUM_OF_FILE/NUM_OF_DIR_ENTRY_PER_BLOCK + 1)
#define SIZE_OF_HASH 10
#define NUM_OF_INODE_PER_BLOCK (SIZE_OF_BLOCK/sizeof(inode)) //一个块中的最大inode数
#define NUM_OF_INODE MAX_NUM_OF_FILE //inode总数
#define NUM_OF_INODE_BLOCK (NUM_OF_INODE/NUM_OF_INODE_PER_BLOCK + 1) //inode块数
#define RESERVED_BLOCK (((3000+NUM_OF_BLOCK_BITMAP+NUM_OF_INODE_BITMAP+NUM_OF_INODE_BLOCK+NUM_OF_INIT_DIR_BLOCK)/SIZE_OF_BLOCK+1)*SIZE_OF_BLOCK)

typedef int word_t;

typedef struct dir_entry
{
    word_t index; //if 0 then file else dir
    char file_name[SIZE_OF_FILE_NAME];
}dir_entry;

typedef struct dir_block
{
    char dir_name[SIZE_OF_FILE_NAME];
    bool is_use[NUM_OF_DIR_ENTRY_PER_BLOCK];
    word_t have_next; 
    word_t next_index; 
    struct dir_entry dir_entry_array[NUM_OF_DIR_ENTRY_PER_BLOCK];
    char addition[SIZE_OF_BLOCK - 504]; 
}dir_block;

typedef struct data_block
{
    word_t next;
    char space[SIZE_OF_BLOCK];
}data_block;

typedef struct super_block
{
    word_t size_of_memory;
    word_t block_size;
    word_t sum_of_inode;
    word_t free_num_of_inode;
    word_t sum_of_block;
    word_t free_num_of_block;
    word_t super_block_index;
    word_t block_bitmap_index;
    word_t inode_bitmap_index;
    word_t inode_index;
    word_t init_dir_index;
    char addition[SIZE_OF_BLOCK - 11 * sizeof(int)]; 
}super_block;

typedef struct block_bitmap
{
    bool is_use[SIZE_OF_BLOCK];
}block_bitmap; 

typedef struct inode_bitmap
{
    bool is_use[SIZE_OF_BLOCK];
}inode_bitmap;

typedef struct inode
{
    word_t size;
    word_t index; 
}inode;

typedef struct hash_s
{
    word_t index;
    char* file_name;
    struct hash_s* before; 
    struct hash_s* next;
}hash_s;

void file_system_cmd();
void load_file_system();
void update_file_system_manager();
void init_file_system();
int read_file(char* name);
int create_file(char* addr_r, char* name_r);
void read_all_files();
int delete_file(char* name);
void load_html(char* url);
//test
void modify_inode();
int read_file_without_outfile(char* name);
void test_speed(char* url);
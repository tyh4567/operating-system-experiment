#include "filemanager.h"

char outaddr[30] = "./outfile/";
//error ? super_block_point等如果使用堆内存，fseek会改变其值？？？？？？
FILE* fp;
super_block super_block_point;
char array_to_read_file[SIZE_OF_DATA + 10];
char file_name_in_init_dir[MAX_NUM_OF_FILE][SIZE_OF_FILE_NAME]; 
word_t use_in_init_dir[MAX_NUM_OF_FILE];
word_t index_in_init_dir[MAX_NUM_OF_FILE];
word_t size_of_index = 0; 
hash_s hash_of_entry[SIZE_OF_HASH];

//should finish dir_entry
void load_file_system()
{
    //初始化hash table
    for(int i = 0; i < SIZE_OF_HASH; i++)
    {
        hash_of_entry[i].before = NULL;
        hash_of_entry[i].next = NULL;
    }

    printf("# 开始加载文件系统\n");
    fp = fopen("./filesystem", "rb+"); 
    if(fp == NULL)
    {
        printf("# 加载文件系统失败\n");
        exit(-1); 
    }

    // printf("# 开始读入超级块\n");
    fread(&super_block_point, sizeof(char), SIZE_OF_BLOCK, fp); 
    fflush(fp);
    printf("  文件系统大小 %dMB\n  块大小 %dByte\n  inode总数 %d\n  空闲inode数 %d\n  总块数 %d\n  空闲块数 %d\n", \
        super_block_point.size_of_memory, super_block_point.block_size, super_block_point.sum_of_inode, \
        super_block_point.free_num_of_inode, super_block_point.sum_of_block, \
        super_block_point.free_num_of_block);
    printf("  超级块索引 %d\n  块位图索引 %d\n  inode位图索引 %d\n  inode表索引 %d\n  根目录索引 %d\n", \
        super_block_point.super_block_index, super_block_point.block_bitmap_index, \
        super_block_point.inode_bitmap_index, super_block_point.inode_index, \
        super_block_point.init_dir_index);
    // printf("# 完成读入超级块\n");

    // printf("# 开始读入init_dir\n");
    fseek(fp, SIZE_OF_BLOCK * (super_block_point.init_dir_index), SEEK_SET);
    int i = 0; 
    dir_block* init_dir_point = (dir_block*)malloc(sizeof(dir_block));
    for(int j = 0; j < NUM_OF_INIT_DIR_BLOCK; j++)
    {
        fread(init_dir_point, 1, SIZE_OF_BLOCK, fp);
        for(int k = 0; k < NUM_OF_DIR_ENTRY_PER_BLOCK; k++)
        {
            if(init_dir_point->is_use[k] == 1)
            {
                strcpy(file_name_in_init_dir[i], init_dir_point->dir_entry_array[k].file_name);
                use_in_init_dir[i] = 1;
                index_in_init_dir[i] = init_dir_point->dir_entry_array[k].index;
                

                hash_s* new_s = (hash_s*)malloc(sizeof(hash_s));
                new_s->file_name = file_name_in_init_dir[i];
                new_s->index = index_in_init_dir[i];
                hash_s* index_s = &hash_of_entry[new_s->file_name[0] % SIZE_OF_HASH];
                while(index_s->next != NULL)
                {
                    index_s = index_s->next;
                }
                index_s->next = new_s; 
                new_s->before = index_s; 
                new_s->next = NULL;

                i++;
            }
        }
    }
    size_of_index = i; 
    //printf("# 完成读入init_dir\n");

    printf("# 完成加载文件系统\n");
}

void update_file_system_manager()
{
    fseek(fp, 0, SEEK_SET);
    fwrite(&super_block_point, sizeof(char), SIZE_OF_BLOCK, fp); 
    fflush(fp);
    // printf("# 开始写入init_dir\n");
    fseek(fp, SIZE_OF_BLOCK * (super_block_point.init_dir_index), SEEK_SET);
    int i = 0; 
    for(int j = 0; j < NUM_OF_INIT_DIR_BLOCK; j++)
    {
        dir_block* init_dir_point = (dir_block*)malloc(sizeof(dir_block));
        //fread(init_dir_point, 1, SIZE_OF_BLOCK, fp);
        int k = 0; 
        // printf("size of index == %d\n", size_of_index); 
        for(k = 0; k < NUM_OF_DIR_ENTRY_PER_BLOCK && i < size_of_index; k++)
        {
            while(use_in_init_dir[i] != 1) i++;
            strcpy(init_dir_point->dir_entry_array[k].file_name, file_name_in_init_dir[i]);
            init_dir_point->is_use[k] = 1;
            init_dir_point->dir_entry_array[k].index = index_in_init_dir[i];
            // printf("  写入文件: %s\n",init_dir_point->dir_entry_array[k].file_name );
            i++;
        }
        while(k < NUM_OF_DIR_ENTRY_PER_BLOCK)
        {
            init_dir_point->is_use[k++] = 0;
        }
        fwrite(init_dir_point, 1, SIZE_OF_BLOCK, fp);
        free(init_dir_point);
    }
    // printf("# 完成写入init_dir\n");
}

int create_file(char* addr_r, char* name_r)
{
    char name[SIZE_OF_FILE_NAME];
    word_t index_of_inode = 0;
    word_t index_of_block = RESERVED_BLOCK;
    word_t num_of_block;
    unsigned long long len_long_long;
    word_t len;
    word_t index_of_dir_entry = 0; 

    strcpy(name, name_r);
    FILE* ready = fopen(addr_r, "rb+"); 
    if(ready == NULL)
    {
        printf("error to open file\n"); 
        return 0;
    }

    fseek(ready, 0, SEEK_END);
    len_long_long = ftell(ready);
    if(len_long_long >= MAX_SIZE_OF_FILE)
    {
        printf("file is too large, its size is %d\n", len); 
        return 0;
    }
    len = len_long_long;
    num_of_block = ceil((double)len / (double)SIZE_OF_DATA);
    if(num_of_block > super_block_point.free_num_of_block)
    {
        printf("no block free\n"); 
        return 0;
    }
    if(super_block_point.free_num_of_inode <= 0)
    {
        printf("no inode free\n"); 
        return 0;
    }
    super_block_point.free_num_of_block -= num_of_block;
    super_block_point.free_num_of_inode -= 1;

    //inode
    fseek(fp, SIZE_OF_BLOCK * (long long)(super_block_point.inode_bitmap_index), SEEK_SET);
    bool is_use = true; 
    for(index_of_inode = 0; index_of_inode < NUM_OF_INODE; index_of_inode++)
    {
        fread(&is_use, 1, sizeof(bool), fp);
        if(is_use == false) //打印值为16??
        {
            is_use = true;
            //printf("%d 号is == %d\n", index_of_inode % SIZE_OF_BLOCK, inode_bitmap_point->is_use[index_of_inode % SIZE_OF_BLOCK]);
            //fflush(stdout); 
            // inode_bitmap_point->is_use[index_of_inode % SIZE_OF_BLOCK] = 1;
            fseek(fp, SIZE_OF_BLOCK * (long long)(super_block_point.inode_bitmap_index) + index_of_inode, SEEK_SET);
            fwrite(&is_use, 1, sizeof(bool), fp); 
            fflush(fp);
            break;
        }
    }
    if(index_of_inode == NUM_OF_INODE)
    {
        printf("no inode free\n"); 
        return 0;
    }
    inode* inode_this = (inode*)malloc(sizeof(inode));
    inode_this->size = len;

    //dir_entry
    while(index_of_dir_entry < NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK)
    {
        if(use_in_init_dir[index_of_dir_entry] == 0) break;
        index_of_dir_entry++;
    }
    if(index_of_dir_entry >= NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK)
    {
        printf("no dir entry free\n"); 
        return 0;
    }
    strcpy(file_name_in_init_dir[index_of_dir_entry], name_r); 
    use_in_init_dir[index_of_dir_entry] = 1; 
    index_in_init_dir[index_of_dir_entry] = index_of_inode;
    size_of_index++; 

    hash_s* new_s = (hash_s*)malloc(sizeof(hash_s));
    new_s->file_name = file_name_in_init_dir[index_of_dir_entry];
    new_s->index = index_of_inode;
    hash_s* index_s = &hash_of_entry[new_s->file_name[0] % SIZE_OF_HASH];
    while(index_s->next != NULL)
    {
        index_s = index_s->next;
    }
    index_s->next = new_s; 
    new_s->before = index_s; 
    new_s->next = NULL;
    // printf("cache write %s\n", file_name_in_init_dir[index_of_dir_entry]);
    // printf("cache index %d\n", index_of_dir_entry);

    //block
    fseek(fp, SIZE_OF_BLOCK * (super_block_point.block_bitmap_index), SEEK_SET);
    int before_index = 0;
    fseek(ready, 0, SEEK_SET);
    for(int i = 0; i < num_of_block; i++)
    {
        for(; index_of_block < NUM_OF_BLOCK; index_of_block++)
        {
            bool is_use = false; 
            fseek(fp, SIZE_OF_BLOCK * (super_block_point.block_bitmap_index) + index_of_block, SEEK_SET);
            fread(&is_use, 1, sizeof(bool), fp); 
            if(is_use == false)
            {
                is_use = true;
                fseek(fp, SIZE_OF_BLOCK * (super_block_point.block_bitmap_index) + index_of_block, SEEK_SET);
                fwrite(&is_use, 1, sizeof(bool), fp); 
                fflush(fp);
                break;
            }
        }
        if(index_of_block == NUM_OF_BLOCK)
        {
            printf("no block free\n"); 
            return 0;
        }
        // printf("get block %d\n", index_of_block);
        if(before_index != 0) 
        {
            fseek(fp, before_index * SIZE_OF_BLOCK, SEEK_SET);
            fwrite(&index_of_block, 1, sizeof(word_t), fp);
            // printf("write next index == %s\n", next_block);
        }
        else
        {
            inode_this->index = index_of_block;
        }

        fseek(fp, index_of_block * SIZE_OF_BLOCK + 4, SEEK_SET);
        char temp[SIZE_OF_DATA] = {0}; 
        fread(temp, 1, SIZE_OF_DATA, ready);
        fwrite(temp, 1, SIZE_OF_DATA, fp);
        before_index = index_of_block;
    }
    fseek(fp, index_of_block * SIZE_OF_BLOCK, SEEK_SET);
    word_t next_block = 0;
    fwrite(&next_block, 1, sizeof(word_t), fp);
    fclose(ready);

    fseek(fp, SIZE_OF_BLOCK * (super_block_point.inode_index) + index_of_inode * sizeof(inode), SEEK_SET);
    fwrite(inode_this, 1, sizeof(inode), fp);

    // update_file_system_manager();
    return 1;
}

int read_file(char* name)
{
    word_t index_of_inode = 0;
    word_t index_of_block = 0;

    //find in file_name_in_init_dir
    // int i_dir;
    // for(i_dir = 0; i_dir < NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK; i_dir++)
    // {
    //     if(strcmp(name, file_name_in_init_dir[i_dir]) == 0) break; 
    // }
    // if(i_dir >= NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK)
    // {
    //     printf("not find in dir entry\n");
    //     exit(-1);
    // }
    //index_of_inode = index_in_init_dir[i_dir]; 

    //find in hash
    hash_s* index_s = &hash_of_entry[name[0] % SIZE_OF_HASH];
    index_s = index_s->next;
    while(index_s != NULL)
    {
        if(strcmp(index_s->file_name, name) == 0) break;
        else index_s = index_s->next; 
    }
    if(index_s == NULL) 
    {
        printf("not find in hash\n");
        return 0; 
    }
    index_of_inode = index_s->index;
    //printf("index_of_inode = %d\n", index_of_inode);
    inode* inode_this = (inode*)malloc(sizeof(inode)); 
    fseek(fp, (super_block_point.inode_index) * SIZE_OF_BLOCK + index_of_inode * sizeof(inode), SEEK_SET);
    fread(inode_this, 1, sizeof(inode), fp);
    word_t len = inode_this->size;
    len %= SIZE_OF_DATA;
    index_of_block = inode_this->index;
    char outfile_addr[50];
    sprintf(outfile_addr, "%s%s", outaddr, name);
    FILE* ftest = fopen(outfile_addr, "wb+");

    while(index_of_block != 0)
    {
        // printf("this block is == %d, inode is == %d\n", index_of_block, index_of_inode);
        fseek(fp, index_of_block * SIZE_OF_BLOCK, SEEK_SET);
        fread(&index_of_block, 1, sizeof(word_t), fp);
        fread(array_to_read_file, 1, SIZE_OF_DATA, fp);
        //test
        if(index_of_block == 0) fwrite(array_to_read_file, 1, len, ftest);
        else fwrite(array_to_read_file, 1, SIZE_OF_DATA, ftest);
    }
    fflush(ftest); 
    return 1;
}

int delete_file(char* name)
{
    word_t index_of_inode = 0;
    word_t index_of_block = 0;

    //delete in hash
    hash_s* index_s = hash_of_entry[name[0] % SIZE_OF_HASH].next;
    while(index_s != NULL)
    {
        if(strcmp(index_s->file_name, name) == 0) break;
        else index_s = index_s->next; 
    }
    if(index_s == NULL) 
    {
        printf("not find in hash\n");
        return 0;
    }
    index_s->before->next = index_s->next;
    if(index_s->next != NULL) index_s->next->before = index_s->before;
    free(index_s);

    int i_dir;
    for(i_dir = 0; i_dir < NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK; i_dir++)
    {
        if(strcmp(name, file_name_in_init_dir[i_dir]) == 0) break; 
    }
    if(i_dir >= NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK)
    {
        printf("not find in dir entry\n");
        return 0;
    }
    index_of_inode = index_in_init_dir[i_dir]; 
    strcpy(file_name_in_init_dir[i_dir], "");
    index_in_init_dir[i_dir] = 0; 

    inode* inode_this = (inode*)malloc(sizeof(inode)); 
    fseek(fp, SIZE_OF_BLOCK * (super_block_point.inode_bitmap_index) + index_of_inode * sizeof(inode), SEEK_SET); 
    fread(inode_this, 1, sizeof(inode), fp); 
    // inode_bitmap_point.is_use[index_of_inode] = 0; 
    fseek(fp, SIZE_OF_BLOCK * (super_block_point.inode_bitmap_index) + index_of_inode, SEEK_SET); 
    bool tr = 0; 
    fwrite(&tr, 1, sizeof(bool), fp); 

    index_of_block = inode_this->index; 
    while(index_of_block != 0)
    {
        // block_bitmap_point[index_of_block / SIZE_OF_BLOCK].is_use[index_of_block % SIZE_OF_BLOCK] = 0;
        fseek(fp, SIZE_OF_BLOCK * (super_block_point.block_bitmap_index) + index_of_block, SEEK_SET); 
        fwrite(&tr, 1, sizeof(bool), fp); 
        fseek(fp, index_of_block * SIZE_OF_BLOCK, SEEK_SET);
        fread(&index_of_block, 1, sizeof(word_t), fp);
    }
    return 1; 
}

void read_all_files()
{
    // int j = 0; 
    // for(int i = 0; i < size_of_index; i++)
    // {
    //     while(use_in_init_dir[j] != 1) j++; 
    //     if(j >= NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK) return; 
    //     printf("%s\n", file_name_in_init_dir[j]); 
    //     j++;
    //     if(j >= NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK) return; 
    // }

    //read in hash
    int sum = 0; 
    for(int i = 0; i < SIZE_OF_HASH; i++)
    {
        hash_s*  in = hash_of_entry[i].next; 
        while(in != NULL)
        {
            printf("%s ", in->file_name); 
            printf("index = %d\n", in->index); 
            in = in->next;
            sum++;
        }
    }
    printf("总数为: %d\n", sum);
}

//test
int read_file_without_outfile(char* name)
{
    word_t index_of_inode = 0;
    word_t index_of_block = 0;

    //find in file_name_in_init_dir
    int i_dir;
    for(i_dir = 0; i_dir < NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK; i_dir++)
    {
        if(strcmp(name, file_name_in_init_dir[i_dir]) == 0) break; 
    }
    if(i_dir >= NUM_OF_DIR_ENTRY_PER_BLOCK * NUM_OF_INIT_DIR_BLOCK)
    {
        // printf("not find in dir entry\n");
        return 0;
    }
    index_of_inode = index_in_init_dir[i_dir]; 

    //find in hash
    // hash_s* index_s = &hash_of_entry[name[0] % SIZE_OF_HASH];
    // index_s = index_s->next;
    // while(index_s != NULL)
    // {
    //     if(strcmp(index_s->file_name, name) == 0) break;
    //     else index_s = index_s->next; 
    // }
    // if(index_s == NULL) 
    // {
    //     // printf("not find in hash\n");
    //     return 0; 
    // }
    // index_of_inode = index_s->index;

    //printf("index_of_inode = %d\n", index_of_inode);
    inode* inode_this = (inode*)malloc(sizeof(inode)); 
    fseek(fp, (super_block_point.inode_index) * SIZE_OF_BLOCK + index_of_inode * sizeof(inode), SEEK_SET);
    fread(inode_this, 1, sizeof(inode), fp);
    word_t len = inode_this->size;
    len %= SIZE_OF_DATA;
    index_of_block = inode_this->index;
    char outfile_addr[50];
    sprintf(outfile_addr, "%s%s", outaddr, name);
    while(index_of_block != 0)
    {
        fseek(fp, index_of_block * SIZE_OF_BLOCK, SEEK_SET);
        fread(&index_of_block, 1, sizeof(word_t), fp);
        fread(array_to_read_file, 1, SIZE_OF_DATA, fp);
    }
    return 1;
}
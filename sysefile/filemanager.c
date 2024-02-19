#include "filemanager.h"

void init_file_system()
{
    printf("# 开始创建文件系统框架\n");
    FILE *fp = fopen("./filesystem", "wb+"); 
    void* begin_memory = malloc(MB_LEN(SIZE_OF_CONTROL));
    char* modify_point = begin_memory; 

    super_block* super_block_point = (super_block*)modify_point;
    word_t super_block_index = 0; 
 
    word_t block_bitmap_index = super_block_index + 1;
    block_bitmap* block_bitmap_point; 
    for(int i = 0; i < NUM_OF_BLOCK_BITMAP; i++)
    {
        modify_point += SIZE_OF_BLOCK;
        block_bitmap_point = (block_bitmap*)modify_point;
        if(i*SIZE_OF_BLOCK <= RESERVED_BLOCK) memset(block_bitmap_point->is_use, 1, SIZE_OF_BLOCK);
        else memset(block_bitmap_point->is_use, 0, SIZE_OF_BLOCK);
    }

    word_t inode_bitmap_index = block_bitmap_index + NUM_OF_BLOCK_BITMAP;
    inode_bitmap* inode_bitmap_point;
    for(int i = 0; i < NUM_OF_INODE_BITMAP; i++)
    {
        modify_point += SIZE_OF_BLOCK;
        inode_bitmap_point = (inode_bitmap*)modify_point;
        memset(inode_bitmap_point->is_use, 0, SIZE_OF_BLOCK);
    }

    word_t inode_index = inode_bitmap_index + NUM_OF_INODE_BITMAP;
    inode* inode_point;
    modify_point += SIZE_OF_BLOCK * NUM_OF_INODE_BLOCK;

    word_t init_dir_index = inode_index + NUM_OF_INODE_BLOCK;
    for(int i = 0; i < NUM_OF_INIT_DIR_BLOCK; i++)
    {
        modify_point += SIZE_OF_BLOCK;
        dir_block* init_dir = (dir_block*)modify_point; 
        init_dir->have_next = 1; 
        init_dir->next_index = init_dir_index + i + 1; 
        memset(init_dir->is_use, 0, NUM_OF_DIR_ENTRY_PER_BLOCK);
    }
    ((dir_block*)modify_point)->have_next = 0; 
    ((dir_block*)modify_point)->next_index = 0;  

    //init_super_block(super_block_point);
    super_block_point->size_of_memory = SIZE_OF_MEMORY;
    super_block_point->block_size = SIZE_OF_BLOCK;
    super_block_point->sum_of_inode = NUM_OF_INODE;
    super_block_point->free_num_of_inode = NUM_OF_INODE;
    super_block_point->sum_of_block = NUM_OF_BLOCK;
    super_block_point->free_num_of_block = NUM_OF_BLOCK - RESERVED_BLOCK;
    super_block_point->block_bitmap_index = block_bitmap_index;
    super_block_point->inode_bitmap_index = inode_bitmap_index; 
    super_block_point->inode_index = inode_index; 
    super_block_point->init_dir_index = init_dir_index; 

    char* file_print = begin_memory;
    for(int i = 0; i < MB_LEN(SIZE_OF_CONTROL); i++)
    {
        fputc(*file_print, fp); 
        file_print++;
    }

    for(int i = 0; i < 20; i++)
    {
        for(int j = 0; j < MB_LEN(SIZE_OF_CONTROL); j++)
        {
            fputc("", fp); 
        }
    }
    fflush(fp);
    fclose(fp);
    printf("# 文件系统框架创建完成, 大小 %d MB\n", SIZE_OF_MEMORY);
}

void file_system_cmd()
{
    int has_load = 0; 
    char com[50]; 
    while(1)
    {
        printf("(FileSystem) ");
        gets(com);
        if(strcmp(com, "init") == 0)
        {
            init_file_system();
            load_file_system();
            has_load = 1; 
            create_file("/home/tyh/syse/sysefile/testfile/test1.txt", "test1.txt");
            create_file("/home/tyh/syse/sysefile/testfile/test2.txt", "test2.txt");
            // create_file("/home/tyh/syse/sysefile/testfile/test3.txt", "test3.txt");
        }
        else if(strcmp(com, "q") == 0)
        {
            if(!has_load) printf("not load\n");
            else update_file_system_manager();
            return; 
        }
        else if(strcmp(com, "ls") == 0)
        {
            if(!has_load) printf("not load\n");
            else read_all_files(); 
        }
        else if(strcmp(com, "touch") == 0)
        {
            char addr[1010]; 
            char name[SIZE_OF_FILE_NAME]; 
            if(!has_load) printf("not load\n");
            else 
            {
                printf("输入地址:"); 
                scanf("%s", addr);
                printf("\n输入新名字:");
                scanf("%s", name);
                printf("\n");
                if(*addr == "" || *name == "")
                {
                    printf("重新输入\n"); 
                }
                create_file(addr, name);
            }
        }
        else if(strcmp(com, "read") == 0)
        {
            if(!has_load) {printf("not load\n"); continue;}
            printf("输入文件名:"); 
            gets(com);
            if(strcmp(com, "") == 0) read_file("test1.txt");
            else read_file(com);
        }
        else if(strcmp(com, "load") == 0)
        {
            load_file_system();
            has_load = 1; 
        }
        else if(strcmp(com, "delete") == 0)
        {
            if(!has_load) {printf("not load\n"); continue;}
            printf("输入文件名:"); 
            gets(com);
            if(strcmp(com, "") == 0)
            {
                printf("未输入文件名\n");
                continue;
            }
            else delete_file(com);
        }
        else if(strcmp(com, "load html") == 0)
        {
            if(!has_load) {printf("not load\n"); continue;}
            load_html("/home/tyh/syse/sysefile/htmlfile/");
        }
        else if(strcmp(com, "test speed") == 0)
        {
            if(!has_load) {printf("not load\n"); continue;}
            test_speed("/home/tyh/syse/sysefile/htmlfile/");
        }
        else
        {
            printf("no command"); 
        }
        printf("\n");
    }
}

void load_html(char* url)
{
    DIR * dir = opendir(url);
    if(dir == NULL)
    {
        printf("打开失败！\n");
    }

    struct dirent * dirp;
    int sum = 0;
    while(1)
    {
        dirp = readdir(dir);
        if(dirp == NULL)
        {
            break;
        }
        char *res = malloc(strlen(dirp->d_name) + strlen(url) + 1);
        if(res == NULL) exit (1);
        strcpy(res, url);
        strcat(res, dirp->d_name);

        //http://localhost:8080/
        char *ls = malloc(strlen(dirp->d_name) + strlen(url) + 1);
        strcpy(ls, "http://localhost:8080/");
        strcat(ls, dirp->d_name);
        strcat(ls, "\n");
        FILE* fls = fopen("./htmltest.txt", "a+");
        fwrite(ls, 1, strlen(ls), fls);
        fclose(fls);
        if(create_file(res, dirp->d_name))
        {
            // printf("# 导入完成: %s\n",res);
            sum++;
        }

    }
    closedir(dir);
    printf("# 导入完成: 总数 %d\n", sum);
}

void test_speed(char* url)
{ 
    int sum = 0;
    DIR* dir = opendir(url);
    if(dir == NULL)
    {
        printf("打开失败！\n");
    }

    struct dirent * dirp;
    struct timeval tv1, tv2;
    gettimeofday(&tv1,NULL);
    while(1)
    {
        dirp = readdir(dir);
        if(dirp == NULL)
        {
            break;
        }
        char *res = malloc(strlen(dirp->d_name) + strlen(url) + 1);
        if(res == NULL) exit (1);
        strcpy(res, url);
        strcat(res, dirp->d_name);
        FILE* ready = fopen(res, "rb+"); 
        
        if(ready == NULL)
        {
            // printf("name is == %s", res);
            // printf("error to open file\n"); 
            continue;
        }
        fseek(ready, 0, SEEK_SET);
        int ret = 0; 
        char buf[1000];
        
        do{
            ret = fread(buf, 1, 800, ready);
        }while(ret != 0);
        sum++;
    }
    gettimeofday(&tv2,NULL);
    printf("本机文件系统时间: %ldus, 读取文件%d个\n", tv2.tv_sec*1000000 + tv2.tv_usec - (tv1.tv_sec*1000000 + tv1.tv_usec), sum); //微秒

    dir = opendir(url);
    sum = 0;
    gettimeofday(&tv1,NULL);
    while(1)
    {
        dirp = readdir(dir);
        if(dirp == NULL)
        {
            break;
        }
        if(read_file_without_outfile(dirp->d_name))
        // if(read_file(dirp->d_name))
        {
            sum++;
        }
    }
    gettimeofday(&tv2,NULL);
    printf("新文件系统时间: %ldus, 读取文件%d个\n", tv2.tv_sec*1000000 + tv2.tv_usec - (tv1.tv_sec*1000000 + tv1.tv_usec), sum); //微秒
    closedir(dir);
}

#include "../include/threadpool.h"
#include "../include/sharedqueue.h"
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_NUM_OF_TASK_IN_THREAD_POOL 100
#define STOP_PROC_SIGNAL 10
#define MAX_PROC 10
#define VERSION 23 
#define BUFSIZE 8096 
#define ERROR         42 
#define LOG            44 
#define FORBIDDEN 403 
#define NOTFOUND   404 
#ifndef SIGCLD 
    #define SIGCLD SIGCHLD 
#endif 
typedef struct proc_status
{
	int proc_id; 
	int proc_len; //the sum of tasks in proc(id)
	int proc_sta; //0:not run, 1:run
    int proc_pipe_id; //pip to read
}proc_status;

typedef struct arg_data
{
	int listenfd; 
}arg_data;

struct Ext_and_type
{
	char *ext;
	char *filetype;
};

struct Ext_and_type extensions[] = {
	{"gif", "image/gif"},
	{"jpg", "image/jpg"},
	{"jpeg", "image/jpeg"},
	{"png", "image/png"},
	{"ico", "image/ico"},
	{"zip", "image/zip"},
	{"gz", "image/gz"},
	{"tar", "image/tar"},
	{"htm", "text/html"},
	{"html", "text/html"},
	{0, 0}};

typedef struct webparam{ 
    int hit; 
    int fd; 
} webparam; 

typedef struct heartbeat_data
{
    threadpool* tp; 
    int fd; 
}heartbeat_data;

typedef struct mgr_data
{
    threadpool* tp; 
}mgr_data;
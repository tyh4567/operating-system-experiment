
#include "../include/threadpool.h"

#define VERSION 23 
#define BUFSIZE 8096 
#define ERROR         42 
#define LOG            44 
#define FORBIDDEN 403 
#define NOTFOUND   404 
#ifndef SIGCLD 
    #define SIGCLD SIGCHLD 
#endif 
void web_send_file(void* data); 
void web_read_file(void* data);
void web_read_msg(void* data); 


typedef struct filename{
	int sockedfd; 
	char buffer[BUFSIZE + 1];
	struct filename* next; 
}filename;

typedef struct msg
{
	int sockedfd; 
	char file_content[BUFSIZ+1]; 
	int count; 
	struct msg* next; 
}msg;

struct filename_queue
{
	int len; 
	filename* front; 
	filename* rear; 
	pthread_mutex_t filename_queue_mutex; 
	staconv has_jobs; 
}filename_queue;

struct msg_queue
{
	int len; 
	msg* front; 
	msg* rear; 
	pthread_mutex_t msg_queue_mutex; 
	staconv has_jobs; 
}msg_queue;

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

typedef struct { 
    int hit; 
    int fd; 
} webparam; 

threadpool* read_msg_thread_pool;
threadpool* read_file_thread_pool;
threadpool* send_msg_thread_pool;
struct filename_queue* fq;
struct msg_queue* mq;

void handle_for_sigpipe()
{
    struct sigaction sa; //信号处理结构体
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;//设置信号的处理回调函数 这个SIG_IGN宏代表的操作就是忽略该信号 
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))//将信号和信号的处理结构体绑定
        return;
}

unsigned long get_file_size(const char *path) 
{ 
    unsigned long filesize = -1; 
    struct stat statbuff; 
    if(stat(path, &statbuff) < 0){ 
    return filesize; 
    }else{ 
    filesize = statbuff.st_size; 
    } 
    return filesize; 
};

void logger(int type, char *s1, char *s2, int socket_fd) 
{
    int fd ; 
    char logbuffer[BUFSIZE*2]; 
    switch (type) { 
	  case  ERROR:  
        (void)sprintf(logbuffer,"ERROR:  %s:%s  Errno=%d  exiting  pid=%d",s1,  s2, errno, getpid()); 
        break; 
    case FORBIDDEN: 
	    (void)write(socket_fd,  "HTTP/1.1  403  Forbidden\nContent-Length:  185\nConnection: close\nContent-Type: 	text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n",271); 
        (void)sprintf(logbuffer,"FORBIDDEN: %s:%s",s1, s2); 
        break; 
    case NOTFOUND: 
	    (void)write(socket_fd,  "HTTP/1.1  404  Not  Found\nContent-Length:  136\nConnection: close\nContent-Type: 	text/html\n\n<html><head>\n<title>404 	Not Found</title>\n</head><body>\n<h1>Not  Found</h1>\nThe  requested  URL  was  not  found  on  this server.\n</body></html>\n",224); 
        (void)sprintf(logbuffer,"NOT FOUND: %s:%s",s1, s2); 
	    break; 
	case LOG: (void)sprintf(logbuffer," INFO: %s:%s:%d",s1, s2,socket_fd); break;   } 
  /* No checks here, nothing can be done with a failure anyway */ 
        if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0)
         {
            (void)write(fd,logbuffer,strlen(logbuffer)); 
            (void)write(fd,"\n",1); 
            (void)close(fd); 
	    } 

}

void web_read_msg(void* data)
{
	int fd; 
    int hit; 
    int j, file_fd, buflen;
    long i, ret, len;
    char * fstr; 
    static char buffer[BUFSIZE + 1]; /* static so zero filled */ 
    webparam *param=(webparam*) data;
    fd=param->fd; 
    hit=param->hit; 

	ret = read(fd, buffer, BUFSIZE); /* 从连接通道中读取客户端的请求消息 */

	if (ret == 0 || ret == -1)
	{ // 如果读取客户端消息失败，则向客户端发送 HTTP 失败响应信息
		logger(FORBIDDEN, "failed to read browser request", "", fd);
	}
	if (ret > 0 && ret < BUFSIZE) /* 设置有效字符串，即将字符串尾部表示为 0 */
		buffer[ret] = 0;
	else
		buffer[0] = 0;
	for (i = 0; i < ret; i++) /* 移除消息字符串中的“CF” 和“LF” 字符*/
		if (buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i] = '*';
	logger(LOG, "request", buffer, hit);
	/*判断客户端 HTTP 请求消息是否为 GET 类型，如果不是则给出相应的响应消息*/
	if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4))
	{
		logger(FORBIDDEN, "Only simple GET operation supported", buffer, fd);
	}
	for (i = 4; i < BUFSIZE; i++)
	{ /* null terminate after the second space to ignore extra stuff */
		if (buffer[i] == ' ')
		{ /* string is "GET URL " +lots of other stuff */
			buffer[i] = 0;
			break;
		}
	}
	for (j = 0; j < i - 1; j++) /* 在消息中检测路径，不允许路径中出现“.” */
		if (buffer[j] == '.' && buffer[j + 1] == '.')
		{
			logger(FORBIDDEN, "Parent directory (..) path names not supported", buffer, fd);
		}
	if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
		/* 如果请求消息中没有包含有效的文件名，则使用默认的文件名 index.html */
		(void)strcpy(buffer, "GET /index.html");
	/* 根据预定义在 extensions 中的文件类型，检查请求的文件类型是否本服务器支持 */
	buflen = strlen(buffer);
	fstr = (char *)0;
	for (i = 0; extensions[i].ext != 0; i++)
	{
		len = strlen(extensions[i].ext);
		if (!strncmp(&buffer[buflen - len], extensions[i].ext, len))
		{
			fstr = extensions[i].filetype;
			break;
		}
	}
	if (fstr == 0)
		logger(FORBIDDEN, "file extension type not supported", buffer, fd);
	
	struct filename* fnew = (struct filename*)malloc(sizeof(struct filename));
	fnew->sockedfd = fd;  
	strcpy(fnew->buffer, buffer); 
	fnew->next = NULL; 

	task* curtask = (task*)malloc(sizeof(task));
	curtask->function = &web_read_file;
	curtask->arg = (void*)fnew; 
	// printf("get2\n");
	addTask2ThreadPool(read_file_thread_pool, curtask);
	free(param); 
	//printf("succeed first\n"); 
}

void web_read_file(void* data)
{
	char buffer[BUFSIZE + 1];
	int fd, file_fd; 
	int hit, len; 
	int ret; 
	char * fstr = NULL; 
	msg* msgnew = (msg*)malloc(sizeof(msg)); 
	msgnew->next = NULL; 
	filename* fnew = (filename*)data;
	fd = fnew->sockedfd; 
	strcpy(buffer, fnew->buffer); 
	if ((file_fd = open(&buffer[5], O_RDONLY)) == -1)
	{ /* 打开指定的文件名*/
		logger(NOTFOUND, "failed to open file", &buffer[5], fd);
	}
	logger(LOG, "SEND", &buffer[5], hit);
	len = (long)lseek(file_fd, (off_t)0, SEEK_END);																								  /* 通过 lseek 获取文件长度*/
	(void)lseek(file_fd, (off_t)0, SEEK_SET);																									  /* 将文件指针移到文件首位置*/
	(void)sprintf(buffer, "HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection:close\nContent-Type: %s\n\n", VERSION, len, fstr); /* Header + a blank line */
	logger(LOG, "Header", buffer, hit);
	(void)write(fd, buffer, strlen(buffer));
	/* 不停地从文件里读取文件内容，并通过 socket 通道向客户端返回文件内容*/

	// while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
	// {
	// 	(void)write(fd, buffer, ret);
	// }
	ret = read(file_fd, buffer, BUFSIZE);
	// fflush(file_fd);
	msgnew->sockedfd = fd; 
	strcpy(msgnew->file_content, buffer); 
	msgnew->next = NULL; 
	// msgnew->count = ret; 
	msgnew->count = BUFSIZE; 
	close(file_fd);
	task* curtask = (task*)malloc(sizeof(task));
	curtask->function = &web_send_file; 
	curtask->arg = (void*)msgnew; 
	addTask2ThreadPool(send_msg_thread_pool, curtask); 
	//printf("succeedsecond\n"); 
	free(fnew); 
}

void web_send_file(void* data)
{
	int ret, fd;
	msg* msgnew = (msg*)data; 
	fd = msgnew->sockedfd; 
	ret = msgnew->count; 
	(void)write(fd, msgnew->file_content, ret);

	usleep(100); /* sleep 的作用是防止消息未发出，已经将此 socket 通道关闭*/

	close(fd);
    free(msgnew); 
	// printf("succeed third\n"); 
}

int main(int argc, char **argv) 
{ 
	handle_for_sigpipe();
	int i, port, listenfd, socketfd, hit;
	socklen_t length;
	static struct sockaddr_in cli_addr;	 /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	read_msg_thread_pool = initThreadPool(1000);
	read_file_thread_pool = initThreadPool(1000);
	send_msg_thread_pool = initThreadPool(1000);
	// fq = (struct filename_queue*)malloc(sizeof(struct filename_queue)); 
	// mq = (struct msg_queue*)malloc(sizeof(struct msg_queue));
	
	// fq->front = NULL; 
	// fq->rear = NULL; 
	// mq->front = NULL; 
	// mq->rear = NULL; 

	// fq->len = 0; 
	// mq->len = 0; 
	// pthread_mutex_init(&(fq->filename_queue_mutex), NULL); 
	// pthread_mutex_init(&(mq->msg_queue_mutex), NULL);

	// fq->has_jobs.status = false; 
	// mq->has_jobs.status = false; 
	// pthread_mutex_init(&(fq->has_jobs.mutex), NULL);
	// pthread_mutex_init(&(mq->has_jobs.mutex), NULL);
	// pthread_cond_init(&(fq->has_jobs.cond), NULL); 
	// pthread_cond_init(&(mq->has_jobs.cond), NULL);

	/*解析命令参数*/
	if (argc < 3 || argc > 3 || !strcmp(argv[1], "-?"))
	{
		(void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
					 "\tnweb is a small and very safe mini web server\n"
					 "\tnweb only servers out file/web pages with extensions named below\n"
					 "\t and only from the named directory or its sub-directories.\n"
					 "\tThere is no fancy features = safe and secure.\n\n"
					 "\tExample:webserver 8181 /home/nwebdir &\n\n"
					 "\tOnly Supports:", 
					 VERSION);
		for (i = 0; extensions[i].ext != 0; i++)
			(void)printf(" %s", extensions[i].ext);
		(void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
					 "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
					 "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n");
		exit(0);
	}
	if (!strncmp(argv[2], "/", 2) || !strncmp(argv[2], "/etc", 5) ||
		!strncmp(argv[2], "/bin", 5) || !strncmp(argv[2], "/lib", 5) ||
		!strncmp(argv[2], "/tmp", 5) || !strncmp(argv[2], "/usr", 5) ||
		!strncmp(argv[2], "/dev", 5) || !strncmp(argv[2], "/sbin", 6))
	{
		(void)printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
		exit(3);
	}
	if (chdir(argv[2]) == -1)
	{
		(void)printf("ERROR: Can't Change to directory %s\n", argv[2]);
		exit(4);
	}


    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		logger(ERROR, "system call", "socket", 0);
	port = atoi(argv[1]);
	if (port < 0 || port > 60000)
		logger(ERROR, "Invalid port number (try 1->60000)", argv[1], 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		logger(ERROR, "system call", "bind", 0);
	if (listen(listenfd, 64) < 0)
		logger(ERROR, "system call", "listen", 0);
        //初始化线程属性，为分离状态 
        pthread_attr_t attr; 
        pthread_attr_init(&attr); 
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); 
        pthread_t pth;             
        
		int max_of_thread_read_msg = 0; 
		int min_of_thread_read_msg = 99;
		int max_of_thread_read_file = 0; 
		int min_of_thread_read_file = 99;
		int max_of_thread_write_file = 0; 
		int min_of_thread_write_file = 99;
		// int max_of_thread = 0; 
		// int min_of_thread = 99;
		int average_of_thread = 0; 
        for(hit=1; ;hit++)
        { 
			// printf("get1\n");
            length = sizeof(cli_addr); 
            if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
                logger(ERROR,"system call","accept",0); 
			// printf("get2\n");
            webparam *param = malloc(sizeof(webparam)); 
            param->hit=hit; 
            param->fd=socketfd; 
			task* curtask = (task*)malloc(sizeof(task)); 
			curtask->function = &web_read_msg; 
			curtask->arg = (void*)param; 
			curtask->next = NULL; 
			// printf("get1\n");
			addTask2ThreadPool(read_msg_thread_pool, curtask);
			int thread_run_read_msg = getNumofThreadWorking(read_msg_thread_pool);
			if(thread_run_read_msg > max_of_thread_read_msg) max_of_thread_read_msg = thread_run_read_msg; 
			if(thread_run_read_msg < min_of_thread_read_msg) min_of_thread_read_msg = thread_run_read_msg;


			int thread_run_read_file = getNumofThreadWorking(read_file_thread_pool);
			if(thread_run_read_file > max_of_thread_read_file) max_of_thread_read_file = thread_run_read_file; 
			if(thread_run_read_file < min_of_thread_read_file) min_of_thread_read_file = thread_run_read_file;

			int thread_run_write_file = getNumofThreadWorking(send_msg_thread_pool);
			if(thread_run_write_file > max_of_thread_write_file) max_of_thread_write_file = thread_run_write_file; 
			if(thread_run_write_file < min_of_thread_write_file) min_of_thread_write_file = thread_run_write_file;
			// printf("get3\n");
			if(hit % 10 == 0)
			{
				printf("平均活跃时间:%dms\n", get_time_all_run()/1000);
				printf("阻塞时间:%dms\n", get_time_all()*1000 - get_time_all_run()/1000);
				printf("readmsg最大线程数:%d\n", max_of_thread_read_msg); 
				printf("readmsg最低线程数:%d\n", min_of_thread_read_msg); 
				printf("readfile最大线程数:%d\n", max_of_thread_read_file); 
				printf("readfile最低线程数:%d\n", min_of_thread_read_file); 
				printf("writefile最大线程数:%d\n", max_of_thread_write_file); 
				printf("writefile最低线程数:%d\n", min_of_thread_write_file); 
				printf("平均线程数:%d\n", get_thread_all()/get_time_all()); 
				printf("消息长度队列0:%d\n", getNUMofTaskQueue(read_msg_thread_pool));
				printf("消息长度队列1:%d\n", getNUMofTaskQueue(read_file_thread_pool));
				printf("消息长度队列2:%d\n", getNUMofTaskQueue(send_msg_thread_pool));
				printf("%d \n%d\n%d\n", thread_run_read_msg ,thread_run_read_file, thread_run_write_file );
			}
        } 
	// destoryThreadPool(pool); 
}


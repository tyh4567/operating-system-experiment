
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

void web(void* data) 
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

	while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
	{
		(void)write(fd, buffer, ret);
	}

	// usleep(10000); /* sleep 的作用是防止消息未发出，已经将此 socket 通道关闭*/
    close(file_fd);

	// (void)sprintf(buffer, "all time is = %lf", all_time);
	// logger(LOG, "all time : ", buffer, hit);

	close(fd);
    free(param); 
} 

int main(int argc, char **argv) 
{ 
	handle_for_sigpipe();
	int i, port, listenfd, socketfd, hit;
	socklen_t length;
	static struct sockaddr_in cli_addr;	 /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	threadpool* pool = initThreadPool(1200);
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
        
        for(hit=1; ;hit++)
        { 
            length = sizeof(cli_addr); 
            if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
                logger(ERROR,"system call","accept",0); 

            webparam *param = malloc(sizeof(webparam)); 
            param->hit=hit; 
            param->fd=socketfd; 
			task* curtask = (task*)malloc(sizeof(task)); 
			curtask->function = &web; 
			curtask->arg = (void*)param; 
			curtask->next = NULL; 
			addTask2ThreadPool(pool, curtask); 
            // if(pthread_create(&pth, &attr, &web, (void*)param)<0)
            // { 
            //     logger(ERROR,"system call","pthread_create",0); 
            // } 
        } 
	// destoryThreadPool(pool); 
}
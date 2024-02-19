#include "../include/webserver.h"

int now_performance; //0 is the max, and bigger means worse
int last_visit_to_cal_performance = 0; 


//初始化进程状态数组
proc_status proc_status_array[MAX_PROC]; 
int sum_of_proc = 0; //当前进程数

void accept_func(void* para);
int create_proc(proc_status* this_status);
void mgr(void* param);
void heartbeat(void* param);
void cal_now_performance();
void manager_func();
void monitor_func();

int listenfd; 

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
	printf("succeed to run web\n");
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
		(void)strcpy(buffer, "GET /index.html");
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
	{ 
		logger(NOTFOUND, "failed to open file", &buffer[5], fd);
	}
	logger(LOG, "SEND", &buffer[5], hit);
	len = (long)lseek(file_fd, (off_t)0, SEEK_END);																								  /* 通过 lseek 获取文件长度*/
	(void)lseek(file_fd, (off_t)0, SEEK_SET);																									  /* 将文件指针移到文件首位置*/
	(void)sprintf(buffer, "HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection:close\nContent-Type: %s\n\n", VERSION, len, fstr); /* Header + a blank line */
	logger(LOG, "Header", buffer, hit);
	(void)write(fd, buffer, strlen(buffer));
	while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
	{
		(void)write(fd, buffer, ret);
	}
	usleep(10000); 
    close(file_fd);
	close(fd);
    free(param);
} 

void exit_sysytem()
{
	printf("succeed to close proc\n");
	exit(0);
}

int main(int argc, char **argv) 
{ 
	handle_for_sigpipe();
	init_shared_queue();
	//初始化线程，为分离状态 
	pthread_attr_t attr; 
	pthread_attr_init(&attr); 
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); 
	pthread_t accept_thread;             
    pthread_t manager_thread;    
	pthread_t monitor_thread; 

	socklen_t length;
	 //listenfd,
	int i, port, socketfd, hit;
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	// static struct sockaddr_in cli_addr;	 /* static = initialised to zeros */

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
		{
			logger(ERROR, "Invalid port number (try 1->60000)", argv[1], 0);
			perror("error to get port\n"); 
			return 0; 
		}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		logger(ERROR, "system call", "bind", 0);
	if (listen(listenfd, 64) < 0)
		logger(ERROR, "system call", "listen", 0);
	//初始化accept线程
	arg_data* param = (arg_data*)malloc(sizeof(arg_data)); 
	param->listenfd = listenfd; 

	if(pthread_create(&accept_thread, &attr, &accept_func, (void*)param)<0)
	{ 
	    logger(ERROR,"system call","accept_func",0); 
		return 0; 
	} 

	//初始化monitor线程
	if(pthread_create(&monitor_thread, &attr, &monitor_func, NULL)<0)
	{ 
	    logger(ERROR,"system call","monitor_func",0); 
		return 0; 
	} 

	//初始化manager线程
	if(pthread_create(&manager_thread, &attr, &manager_func, NULL)<0)
	{ 
	    logger(ERROR,"system call","manager_func",0); 
		return 0; 
	} 


	signal(SIGINT, exit_sysytem);

	//结束处理
	while(1)
	{
	}

}

void accept_func(void* para)
{
	arg_data* para_arg_data = (arg_data*)para; 
	int listenfd = para_arg_data->listenfd; 
	// int argc = para_arg_data->argc; 
	// char **argv = para_arg_data->argv;

	// socklen_t length;
	
	// int i, port, listenfd, socketfd, hit;
	// static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	// static struct sockaddr_in cli_addr;	 /* static = initialised to zeros */

	// /*解析命令参数*/
	// if (argc < 3 || argc > 3 || !strcmp(argv[1], "-?"))
	// {
	// 	(void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
	// 				 "\tnweb is a small and very safe mini web server\n"
	// 				 "\tnweb only servers out file/web pages with extensions named below\n"
	// 				 "\t and only from the named directory or its sub-directories.\n"
	// 				 "\tThere is no fancy features = safe and secure.\n\n"
	// 				 "\tExample:webserver 8181 /home/nwebdir &\n\n"
	// 				 "\tOnly Supports:", 
	// 				 VERSION);
	// 	for (i = 0; extensions[i].ext != 0; i++)
	// 		(void)printf(" %s", extensions[i].ext);
	// 	(void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
	// 				 "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
	// 				 "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n");
	// 	exit(0);
	// }
	// if (!strncmp(argv[2], "/", 2) || !strncmp(argv[2], "/etc", 5) ||
	// 	!strncmp(argv[2], "/bin", 5) || !strncmp(argv[2], "/lib", 5) ||
	// 	!strncmp(argv[2], "/tmp", 5) || !strncmp(argv[2], "/usr", 5) ||
	// 	!strncmp(argv[2], "/dev", 5) || !strncmp(argv[2], "/sbin", 6))
	// {
	// 	(void)printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
	// 	exit(3);
	// }
	// if (chdir(argv[2]) == -1)
	// {
	// 	(void)printf("ERROR: Can't Change to directory %s\n", argv[2]);
	// 	exit(4);
	// }

    // if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	// 	logger(ERROR, "system call", "socket", 0);
	// port = atoi(argv[1]);
	// if (port < 0 || port > 60000)
	// 	{
	// 		logger(ERROR, "Invalid port number (try 1->60000)", argv[1], 0);
	// 		perror("error to get port\n"); 
	// 		return 0; 
	// 	}
	// serv_addr.sin_family = AF_INET;
	// serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// serv_addr.sin_port = htons(port);
	// if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	// 	logger(ERROR, "system call", "bind", 0);
	// if (listen(listenfd, 64) < 0)
	// 	logger(ERROR, "system call", "listen", 0);
	socklen_t length;
	int socketfd; 
	static struct sockaddr_in cli_addr;	 /* static = initialised to zeros */
	for(int hit=1; ;hit++)
	{ 
		length = sizeof(cli_addr); 
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			logger(ERROR,"system call","accept",0); 
		last_visit_to_cal_performance = time((time_t *)NULL); 
		shared_queue_class* new_sqc = (shared_queue_class*)malloc(sizeof(shared_queue_class));
		new_sqc->fd = socketfd; 
		new_sqc->hit = hit; 
		new_sqc->this_id = 0; 
		new_sqc->next_id = 0; 
		push_task_to_shared_queue(new_sqc); 
		// (void)close(socketfd);
		printf("# succeed to push shared queue\n");
	} 
}

void manager_func()
{
	for(int i = 0; i < MAX_PROC; i++)
	{
		proc_status_array[i].proc_id = 0; 
		proc_status_array[i].proc_len = 0; 
		proc_status_array[i].proc_sta = 0; 
		proc_status_array[i].proc_pipe_id = 0; 
	}

	//test module
	// proc_status_array[0].proc_id = 0; 
	// proc_status_array[0].proc_len = 0; 
	// proc_status_array[0].proc_sta = 1; 
	// proc_status_array[0].proc_pipe_id = 0; 
	// create_proc(&proc_status_array[0]);
	while(1)
	{
		cal_now_performance();
		int len_of_shared_queue = get_shared_queue_len(); 
		if(len_of_shared_queue > 1 && now_performance < 10 && sum_of_proc < MAX_PROC)
		{
			sum_of_proc++;
			printf("# begin to create proc\n");
			int pid;
			int succeed_to_replace = 0; 
			int i = 0; 
			for(i = 0; i < MAX_PROC; i++)
			{
				if(proc_status_array[i].proc_sta == 0)
				{
					proc_status_array[i].proc_len = 0; 
					proc_status_array[i].proc_sta = 1; 
					succeed_to_replace = 1; 
					break; 
				}
			}  

			if(!succeed_to_replace)
			{
				perror("error to replace proc_status_array\n"); 
				exit(-1); 
			}
			create_proc(&proc_status_array[i]);
			printf("# succeed to create proc sum ==%d\n", sum_of_proc);
			
		}
		
		if(len_of_shared_queue <= 0 && now_performance > 10)
		{
			for(int i = 0; i < sum_of_proc; i++)
			{
				if(proc_status_array[i].proc_len == 0)
				{
					kill(proc_status_array[i].proc_id, STOP_PROC_SIGNAL);
					proc_status_array[i].proc_sta = 0;  
					break; 
				}
			}
		}
		sleep(1);
	}
}

void monitor_func()
{
	while(1)
	{
		for(int i = 0; i < MAX_PROC; i++)
		{
			if(proc_status_array[i].proc_sta == 1)
			{
				proc_status* ps = (proc_status*)malloc(sizeof(proc_status));
				read(proc_status_array[i].proc_pipe_id, ps, sizeof(proc_status));
				proc_status_array[i].proc_len = ps->proc_len; 
			}
		}
	}
}

void cal_now_performance()
{
	int now_time = time((time_t *)NULL); 
	int ref = now_time - last_visit_to_cal_performance;
	if(ref <= 0) now_performance = 0; 
	else now_performance = ref;
}

int create_proc(proc_status* this_status)
{
	int pipefd[2]; 
	int res = pipe(pipefd);
	if (res == -1) {
		perror("Unable to create pipe\n");
		return 0;
	}
	this_status->proc_pipe_id = pipefd[0];
	// printf("# succeed to create pipe, fd == %d\n", pipefd[0]);
	int pid = fork(); 
	if(pid == 0)
	{
		close(listenfd);
		signal(STOP_PROC_SIGNAL, exit_sysytem);
		printf("# succeed to run child fork\n");
		//proc status
		proc_status* ps = (proc_status*)malloc(sizeof(proc_status)); 
		ps->proc_len = 0; 
		ps->proc_pipe_id = pipefd[0]; 
		ps->proc_sta = 1; 
		write(pipefd[1], ps, sizeof(proc_status)); 

		threadpool* tp = initThreadPool(10);;
		printf("# succeed to init pool\n");
		//初始化线程参数
		pthread_attr_t attr; 
		pthread_attr_init(&attr); 
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); 
		pthread_t heartbeat_thread;
		pthread_t mgr_thread;


		// printf("# succeed to init proc\n");

		//heartbeat
		heartbeat_data* param1 = (heartbeat_data*)malloc(sizeof(heartbeat_data));
		param1->fd = pipefd[1]; 
		param1->tp = tp; 
		if(pthread_create(&heartbeat_thread, &attr, &heartbeat, (void*)param1) < 0)
		{
			perror("error to create heartbeat\n"); 
			exit(-1);
		}
		printf("# succeed to create heartbeat\n");

		//mgr
		mgr_data* param2 = (mgr_data*)malloc(sizeof(mgr_data));
		param2->tp = tp; 
		if(pthread_create(&mgr_thread, &attr, &mgr, (void*)param2) < 0)
		{
			perror("error to create mgr\n"); 
			exit(-1);
		}
		printf("# succeed to create mgr\n");
		while(1){};
	}
	else if(pid > 0)
	{
		this_status->proc_id = pid; 
		this_status->proc_pipe_id = pipefd[0];
		return 1; 
	}
	else 
	{
		perror("error to fork\n");
		exit(-1); 
	}
}

void heartbeat(void* param)
{
	heartbeat_data* hdparam = (heartbeat_data*)param; 
	threadpool* tp = hdparam->tp; 
	int fd = hdparam->fd; 
	free(param);

	while(1)
	{
		int len = tp->queue.len; 
		proc_status* ps = (proc_status*)malloc(sizeof(proc_status)); 
		ps->proc_id = -1; 
		ps->proc_len = len; 
		ps->proc_pipe_id = -1; 
		ps->proc_sta = 1; 
		write(fd, ps, sizeof(proc_status)); 
		// free(ps);
		usleep(10); 
	}
}

void mgr(void* param)
{
	mgr_data* mgrparam = (mgr_data*)param; 
	
	threadpool* tp = mgrparam->tp; 
	printf("# succeed to run mgr\n");
	while(1)
	{
		if(tp->queue.len < MAX_NUM_OF_TASK_IN_THREAD_POOL)
		{
			shared_queue_class* sqc = (shared_queue_class*)malloc(sizeof(shared_queue_class));
			if(!take_task_from_shared_queue(sqc)) {usleep(100); continue;}

			webparam *param = (webparam*)malloc(sizeof(webparam)); 
            param->hit=sqc->hit; 
            param->fd=sqc->fd; 
			free(sqc);
			task* curtask = (task*)malloc(sizeof(task)); 
			curtask->function = &web; 
			curtask->arg = (void*)param; 
			curtask->next = NULL; 
			addTask2ThreadPool(tp, curtask); 
			printf("# succeed to add task, the fd is ==%d\n", ((webparam*)(curtask->arg))->fd);
		}
	}

}
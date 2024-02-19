#include "../include/util.h"
long time_have_run = 0; 
typedef union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
}semun_def;

#define shm_key_sr 1234
#define shm_key_sw 1235
#define shm_key_wr 1236
#define shm_key_lw 1237

#define sem_key_sr 1034
#define sem_key_sw 1035
#define sem_key_wr 1036
#define sem_key_lw 1037

int shm_id_sr;
int shm_id_sw;
int shm_id_wr;
int shm_id_lw;

int sem_id_sr;
int sem_id_sw;
int sem_id_wr;
int sem_id_lw;

void init_io_e_h()
{
	sem_id_sr = semget(sem_key_sr,1, IPC_CREAT);
    if(sem_id_sr < 0)
    {             
          perror("semget");            
          return 0;   
    }
    semun_def sem_union;    
    sem_union.val = 1;
    if(semctl(sem_id_sr, 0, SETVAL, sem_union)==-1)
    {
        perror("seminit");     
        return 0;
    }

	sem_id_sw = semget(sem_key_sw,1, IPC_CREAT);
    if(sem_id_sw < 0)
    {             
          perror("semget");            
          return 0;   
    }   
    sem_union.val = 1;
    if(semctl(sem_id_sw, 0, SETVAL, sem_union)==-1)
    {
        perror("seminit");     
        return 0;
    }

	sem_id_wr = semget(sem_key_wr,1, IPC_CREAT);
    if(sem_id_wr < 0)
    {             
          perror("semget");            
          return 0;   
    }
  
    sem_union.val = 1;
    if(semctl(sem_id_wr, 0, SETVAL, sem_union)==-1)
    {
        perror("seminit");     
        return 0;
    }

	sem_id_lw = semget(sem_key_lw,1, IPC_CREAT);
    if(sem_id_lw < 0)
    {             
          perror("semget");            
          return 0;   
    }
  
    sem_union.val = 1;
    if(semctl(sem_id_lw, 0, SETVAL, sem_union)==-1)
    {
        perror("seminit");     
        return 0;
    }

	void* addr;
	shm_id_sr = shmget(shm_key_sr, sizeof(long), IPC_CREAT | IPC_EXCL);
	if(shm_id_sr < 0) 
	{
		perror("shm_init");
	}
	addr = shmat(shm_id_sr, 0, 0);
	*(long*)addr = 0;
	shmdt(addr);

	shm_id_sw = shmget(shm_key_sw, sizeof(long), IPC_CREAT | IPC_EXCL);
	if(shm_id_sw < 0) 
	{
		perror("shm_init");
	}
	addr = shmat(shm_id_sw, 0, 0);
	*(long*)addr = 0;
	shmdt(addr);

	shm_id_wr = shmget(shm_key_wr, sizeof(long), IPC_CREAT | IPC_EXCL);
	if(shm_id_wr < 0) 
	{
		perror("shm_init");
	}
	addr = shmat(shm_id_wr, 0, 0);
	*(long*)addr = 0;
	shmdt(addr);

	shm_id_lw = shmget(shm_key_lw, sizeof(long), IPC_CREAT | IPC_EXCL);
	if(shm_id_lw < 0) 
	{
		perror("shm_init");
	}
	addr = shmat(shm_id_lw, 0, 0);
	*(long*)addr = 0;
	shmdt(addr);

}

int sign_wait(int sign_id)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sign_id,&sem_b,1)==-1)
    {
        perror("wait");
        return 0;
    }
    return 1;
}

int sign_signal(int sign_id)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sign_id,&sem_b,1)==-1)
    {
        perror("signael");
        return 0;
    }
    return 1;
}

void send_to_shm(int sign_id, int shmem_id, int times)
{
	while(1)
	{
		if(sign_wait(sign_id))
			break;
	}

    void* addr = shmat(shmem_id, 0, 0);
    long ls = *(long*)addr;
    ls += times;
    *(long*)addr = ls;
    shmdt(addr);

	sign_signal(sign_id);
}

void p_shm(int sign_id, int shmem_id)
{
	while(1)
	{
		if(sign_wait(sign_id))
			break;
	}

    void* addr = shmat(shmem_id, 0, 0);
    long ls = *(long*)addr;
    if(sign_id == sem_id_sr) printf("读socket,时间:%ldms\n", ls); 
    else if(sign_id == sem_id_sw) printf("写socket,时间:%ldms\n", ls); 
    else if(sign_id == sem_id_wr) 
    {
        void* addr2 = shmat(shm_id_sw, 0, 0); 
        long ls2 = *(long*)addr2;
        ls = ls - ls2; 
        printf("读网页,时间:%ldms\n", ls); 
    }
    else if(sign_id == sem_id_lw) printf("写日志,时间:%ldms\n", ls); 
    shmdt(addr);

	sign_signal(sign_id);
}

void print_io_time()
{
	p_shm(sem_id_sr, shm_id_sr); 
	p_shm(sem_id_sw, shm_id_sw); 
	p_shm(sem_id_wr, shm_id_wr); 
	p_shm(sem_id_lw, shm_id_lw); 
}



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

void logger(int type, char *s1, char *s2, int socket_fd)
{
  clock_t startc, endc; 
  startc = clock();
  int fd ;
  char logbuffer[BUFSIZE*2];
  switch (type) {
  case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid());
    break;
  case FORBIDDEN:
    (void)write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n",271);
    (void)sprintf(logbuffer,"FORBIDDEN: %s:%s",s1, s2);
    break;
  case NOTFOUND:
    (void)write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
    (void)sprintf(logbuffer,"NOT FOUND: %s:%s",s1, s2);
    break;
  case LOG: (void)sprintf(logbuffer,"INFO: %s:%s:%d",s1, s2,socket_fd); break;
  }
  /* No checks here, nothing can be done with a failure anyway */
  if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) 
  { 
    time_t timep;   
    startc = clock();  
    time (&timep); 
    char st[1010];
    
    (void)write(fd, "NOW TIME IS :", 13);
    (void)write(fd, asctime(gmtime(&timep)), strlen(asctime(gmtime(&timep))));
    (void)write(fd,logbuffer,strlen(logbuffer));
    (void)write(fd,"\n",1);
    endc = clock();
    double timees = (double)(endc-startc)/1000;
	send_to_shm(sem_id_lw, shm_id_lw, (long)(endc-startc));
    // sprintf(st, "%lf", timees);
    // (void)write(fd,"\nLog_write_time_is",sizeof("\nLog_write_time_is"));
    // (void)write(fd,st,7);
    // (void)write(fd,"\n",1);
    (void)close(fd);
  }

}

void web(int fd, int hit)
{
	
    if(time_have_run == 1000) 
        print_io_time();
	clock_t startcc, endcc; 
    startcc = clock();
	int j, file_fd, buflen;
	long i, ret, len;
	char *fstr;
	static char buffer[BUFSIZE + 1]; /* 设置静态缓冲区 */

//time
	clock_t start_sr, end_sr; 
	start_sr = clock(); 
	ret = read(fd, buffer, BUFSIZE); /* 从连接通道中读取客户端的请求消息 */
	end_sr = clock();
	send_to_shm(sem_id_sr, shm_id_sr, (long)(end_sr-start_sr));

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

//time
	long ss_ti; 
	clock_t start_sa, end_sa; 
	start_sa = clock(); 
	while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
	{
		clock_t start_sw, end_sw; 
		start_sw = clock(); 
		(void)write(fd, buffer, ret);
		end_sw = clock();
		send_to_shm(sem_id_sw, shm_id_sw, (long)(end_sw-start_sw));
		ss_ti += end_sw-start_sw;
	}
	end_sa = clock();
	long lsls = end_sa - start_sa; 
	send_to_shm(sem_id_wr, shm_id_wr, lsls);

	usleep(100); /* sleep 的作用是防止消息未发出，已经将此 socket 通道关闭*/
	endcc = clock();
	double timees = (double)(endcc-startcc)/1000;
	(void)sprintf(buffer, "Web_consumed_time = %lf", timees);
	logger(LOG, "Time", buffer, hit);
	
	// while(1)
	// {
	// 	if(sem_wait())
	// 		break;
	// }

	// shm_send_time(timees);
	// double all_time = shm_read();
	// printf("this is %lf\n", shm_read());
	// printf("all_time is %f\n", shm_read());
	// sem_signal();

	// (void)sprintf(buffer, "all time is = %lf", all_time);
	// logger(LOG, "all time : ", buffer, hit);
	exit(0);

	close(fd);
}

void handle_for_sigpipe()
{
    struct sigaction sa; //信号处理结构体
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;//设置信号的处理回调函数 这个SIG_IGN宏代表的操作就是忽略该信号 
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))//将信号和信号的处理结构体绑定
        return;
}

int main(int argc, char **argv)
{
	handle_for_sigpipe();
	init_io_e_h();

	int i, port, listenfd, socketfd, hit;
	socklen_t length;
	static struct sockaddr_in cli_addr;	 /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	sem_init();
	shm_init();
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
	/* 建立服务端侦听 socket*/
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

	// for (hit = 1;; hit++)
	// {
	// 	length = sizeof(cli_addr);
	// 	if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
	// 		logger(ERROR, "system call", "accept", 0);
	// 	web(socketfd, hit); /* never returns */
	// }

	for(hit=1;;hit++)
	{
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd,(struct sockaddr *)&cli_addr,&length))<0)
		{
			logger(ERROR, "system call", "accept", 0);
		}
		else
		{
			time_have_run++; 
			int pid = fork();
			if(pid < 0)
			{
				logger(ERROR, "system call", "fork", 0);
			}
			else if(pid == 0)
			{
				close(listenfd);
				clock_t startc, endc; 
				web(socketfd,hit);
				endc = clock();
				double ti = (double)(endc-startc)/1000;
			}
			else 
			{
				(void)close(socketfd);
			}
		}
	}
}


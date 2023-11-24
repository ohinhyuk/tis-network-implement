#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 100
#define NAME_SIZE 20
	
void *send_msg(void * arg);
void *recv_msg(void * arg);
void error_handling(char * msg);
	
char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
	
	
void send_image(int sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("File open error");
        return;
    }

    // 파일 크기 구하기
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 파일 크기 전송
    write(sock, &filesize, sizeof(filesize));

    // 파일 데이터 전송
    char buffer[1024];
    while (1) {
        size_t nread = fread(buffer, 1, sizeof(buffer), file);
        if (nread > 0) {
            write(sock, buffer, nread);
        }
        if (nread < sizeof(buffer)) {
            if (feof(file)) break;
            if (ferror(file)) {
                perror("File read error");
                break;
            }
        }
    }
    fclose(file);
}

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if (argc != 4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	 }
	
	sprintf(name, "[%s]", argv[3]);
	sock = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	  
	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);  
	return 0;
}
	
void *send_msg(void * arg)   // send thread main
{
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	while (1) 
	{
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg,"q\n") || !strcmp(msg,"Q\n")) 
		{
			close(sock);
			exit(0);
		}
		else if(!strcmp(msg, "file_share:\n")) {
			printf("디버깅");
			send_image(sock, "./image.jpg");
		}
		sprintf(name_msg,"%s %s", name, msg);
		write(sock, name_msg, strlen(name_msg));
	}
	return NULL;
}
	
void *recv_msg(void * arg)   // read thread main
{
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	int str_len;
	while (1)
	{
		str_len = read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
		if (str_len == -1) 
			return (void*)-1;
		name_msg[str_len] = 0;
		fputs(name_msg, stdout);
	}
	return NULL;
}
	
void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

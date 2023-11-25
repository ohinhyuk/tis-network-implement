#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_SIZE 100
#define NAME_SIZE 256

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);
void send_file(SSL *ssl, const char *filename);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;
    SSL_CTX *ctx;
    SSL *ssl;

    if (argc != 4) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    ctx = SSL_CTX_new(TLS_client_method());
    if (ctx == NULL) {
        error_handling("SSL_CTX_new() error");
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);   
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error!");

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    if (SSL_connect(ssl) != 1)
        error_handling("SSL_connect() error");
    else 
        printf("Connected to the server.\n");
    

    pthread_create(&snd_thread, NULL, send_msg, (void*)ssl);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)ssl);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    // 자원 정리
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    return 0;
}

void *send_msg(void *arg) {
    SSL *ssl = (SSL*)arg;
    char name_msg[NAME_SIZE + BUF_SIZE];
    while (1) {
        fgets(msg, BUF_SIZE, stdin);
        if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
            SSL_shutdown(ssl);
            exit(0);
        } else if (strncmp(msg, "file_share:", 11) == 0) {
            char filename[BUF_SIZE];
            sscanf(msg, "file_share: %s", filename);
            send_file(ssl, filename);
        } else {
            sprintf(name_msg, "%s %s", name, msg);
            SSL_write(ssl, name_msg, strlen(name_msg));
        }
    }
    return NULL;
}


void *recv_msg(void *arg) {
    SSL *ssl = (SSL*)arg;
    char buf[BUF_SIZE];
    int str_len;
    const char *dir_name = "/root/img/";

    mkdir(dir_name, 0777);

    while (1) {
        str_len = SSL_read(ssl, buf, BUF_SIZE - 1);
        if (str_len <= 0)
            break;

        buf[str_len] = 0;

        if (strncmp(buf, "file:", 5) == 0) {
            char filename[BUF_SIZE], file_path[BUF_SIZE];
            long filesize;
            sscanf(buf, "file:%[^:]:%ld:", filename, &filesize);

            snprintf(file_path, sizeof(file_path), "%s%s", dir_name, filename);

            FILE *file = fopen(file_path, "wb");
            if (file == NULL) {
                printf("Cannot open file %s\n", file_path);
                continue;
            }

            int remain_data = filesize;
            while (remain_data > 0) {
                int len = SSL_read(ssl, buf, BUF_SIZE);
                fwrite(buf, 1, len, file);
                remain_data -= len;
            }

            fclose(file);
            printf("File %s received\n", file_path);
        } else {
            
            fputs(buf, stdout);
        }
    }
    return NULL;
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void send_file(SSL *ssl, const char *filename) {
     FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Cannot open file %s\n", filename);
        return;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char fileinfo[BUF_SIZE];
    sprintf(fileinfo, "file:%s:%ld:", filename, filesize);
    SSL_write(ssl, fileinfo, strlen(fileinfo));

    char buffer[BUF_SIZE];
    while (1) {
        size_t nread = fread(buffer, 1, BUF_SIZE, file);
        if (nread > 0) {
            SSL_write(ssl, buffer, nread);
        }
        if (nread < BUF_SIZE) {
            if (feof(file)) 
                break;
            if (ferror(file)) {
                printf("Error reading file\n");
                break;
            }
        }
    }
    fclose(file);
}

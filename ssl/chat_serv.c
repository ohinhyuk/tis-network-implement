#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void *handle_clnt(void *arg);
void send_msg(SSL *ssl, char *msg, int len);
void error_handling(char *msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
SSL_CTX *ctx; // SSL 컨텍스트는 전역으로 선언될 수 있습니다.
pthread_mutex_t mutx;

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;

    // SSL 라이브러리 초기화
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        error_handling("SSL_CTX_new() error");
    }
    if (SSL_CTX_use_certificate_file(ctx, "./ssl/server.crt", SSL_FILETYPE_PEM) != 1) {
        error_handling("SSL_CTX_use_certificate_file() error");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "./ssl/server.key", SSL_FILETYPE_PEM) != 1) {
        error_handling("SSL_CTX_use_PrivateKey_file() error");
    }

    // 나머지 서버 설정...

    // 클라이언트 처리 루프
    while (1) {
        // 클라이언트 연결 처리...
    }

    // 서버 종료 시 자원 정리
    SSL_CTX_free(ctx);
    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg) {
    int clnt_sock = *((int*)arg);
    SSL *ssl = SSL_new(ctx); // 새로운 SSL 객체 생성
    SSL_set_fd(ssl, clnt_sock);
    if (SSL_accept(ssl) != 1) {
        error_handling("SSL_accept() error");
    }

    int str_len = 0;
    char msg[BUF_SIZE];

    while ((str_len = SSL_read(ssl, msg, sizeof(msg))) != 0) {
        send_msg(ssl, msg, str_len);
    }

    // 클라이언트 연결 종료 처리...
    SSL_free(ssl); // SSL 객체 해제
    return NULL;
}

void send_msg(SSL *ssl, char *msg, int len) { // SSL 객체를 인자로 받음
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        SSL *ssl_client = SSL_new(ctx); // 각 클라이언트에 대한 SSL 객체 생성
        SSL_set_fd(ssl_client, clnt_socks[i]);
        SSL_write(ssl_client, msg, len);
        SSL_free(ssl_client); // SSL 객체 해제
    }
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

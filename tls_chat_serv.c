#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void *handle_clnt(void *arg);
void send_msg_to_all(SSL *ssl, char *msg, int len);
void error_handling(char *msg);
SSL_CTX *create_server_context();
void configure_context(SSL_CTX *ctx);

int clnt_cnt = 0;
SSL *clnt_ssl[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
    SSL_CTX *ctx;

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    ctx = create_server_context();
    configure_context(ctx);

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, clnt_sock);
        if (SSL_accept(ssl) == -1) {
            error_handling("SSL_accept() error");
            SSL_free(ssl);
            close(clnt_sock);
            continue;
        }

        pthread_mutex_lock(&mutx);
        clnt_ssl[clnt_cnt++] = ssl;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, ssl);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }

    SSL_CTX_free(ctx);
    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg) {
    SSL *ssl = (SSL*)arg;
    char msg[BUF_SIZE];
    int str_len;

    while ((str_len = SSL_read(ssl, msg, sizeof(msg))) != 0)
        send_msg_to_all(ssl, msg, str_len);

    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        if (ssl == clnt_ssl[i]) {
            while (i < clnt_cnt - 1) {
                clnt_ssl[i] = clnt_ssl[i + 1];
                i++;
            }
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    SSL_free(ssl);
    return NULL;
}

void send_msg_to_all(SSL *ssl, char *msg, int len) {
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) SSL_write(clnt_ssl[i], msg, len);
            
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

SSL_CTX *create_server_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

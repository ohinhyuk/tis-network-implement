CC=gcc
CFLAGS=-Wall -g
LIBS=-lssl -lcrypto -lpthread

all: tls_chat_serv tls_chat_clnt

tls_chat_serv: 
	$(CC) $(CFLAGS) tls_chat_serv.c -o tls_chat_serv $(LIBS)

tls_chat_clnt: 
	$(CC) $(CFLAGS) tls_chat_clnt.c -o tls_chat_clnt $(LIBS)

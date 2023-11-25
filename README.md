# tis-network-implement

## TLS Chat

TLS Chat은 OpenSSL을 사용하여 만든 보안 채팅 프로그램입니다.
이 프로그램은 서버(tls_chat_serv)와 클라이언트(tls_chat_clnt)로 구성되어 있으며, TLS를 통해 암호화된 메시지와 이미지를 주고받을 수 있습니다.

### 기능

- 메시지 전송 및 수신
- 파일 공유
- TLS/SSL을 통한 데이터 암호화

## Setting

프로그램을 사용하기 전에 OpenSSL 라이브러리가 시스템에 설치되어 있어야 합니다. 또한, 서버용 SSL 인증서(server.crt)와 개인 키(server.key)가 필요합니다.

실행

### 컴파일

Makefile을 사용하여 서버와 클라이언트를 컴파일합니다

```
make
```

개별적으로 컴파일하려면 다음 명령어를 사용합니다

```
make tls_chat_serv
make tls_chat_clnt
```

### 실행 방법

1. 서버 실행
   다음 명령어로 서버를 실행합니다

```
./tls_chat_serv
```

2. 클라이언트 실행
   다음 명령어로 클라이언트를 실행합니다:

```
./tls_chat_clnt <server_ip> <port> <nickname>
```

### 버전

OpenSSL 3.1.1

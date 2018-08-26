#ifndef KUKSOCK_H
#define KUKSOCK_H

#include <sys/types.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define ENDFLAG 0xff
typedef struct kuk_sock{
	int fd;
	int clnt;
	uint32_t data_idx;
	uint32_t data_lsize;
	uint32_t data_size;
	uint32_t recv_len;
	char *data;
	struct sockaddr_in sin;
	struct sockaddr_in c_sin;
	char disconnect_flag;
	char start_flag;
	char **p_data;//parsed data
	void *(*decoder)(struct kuk_sock *, void*(*after_decode)(struct kuk_sock*));
	void *(*after_decode)(struct kuk_sock*);
}kuk_sock;

kuk_sock* kuk_sock_init(uint32_t buffersize, void *(*decoder)(kuk_sock*,void*(*ad)(kuk_sock*)),void*(*after_decode)(kuk_sock*));
int kuk_open(kuk_sock*,char *, uint32_t);
int kuk_bind(kuk_sock*);
int kuk_listen(kuk_sock*,int);
int kuk_accept(kuk_sock*);
int kuk_connect(kuk_sock*,char *,uint32_t);
int kuk_service(kuk_sock *, char block);
int kuk_ack2clnt(kuk_sock*);
int kuk_send_buf(kuk_sock *,char *data,int encoded_size,int special_size);
int kuk_request(kuk_sock *,int);
int kuk_send(kuk_sock*,char *,uint32_t);
int kuk_recv(kuk_sock*,char *,uint32_t);

void * kuk_decoder_exp(kuk_sock *, void*(*ad)(kuk_sock*));
#endif

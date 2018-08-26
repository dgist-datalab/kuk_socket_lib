#include "kuk_sock.h"
#include <signal.h>
kuk_sock *kuk_sock_init(uint32_t b_size, void*(*decoder)(kuk_sock*,void*(*ad)(kuk_sock*)),void*(*ad)(kuk_sock*)){
	kuk_sock *res=(kuk_sock*)malloc(sizeof(kuk_sock));
	res->data_lsize=0;
	res->data_size=b_size;
	res->data_idx=b_size;
	res->data=(char*)malloc(b_size);
	memset(res->data,0,b_size);
	res->decoder=decoder;
	res->disconnect_flag=0;
	res->p_data=NULL;
	res->fd=res->clnt=-1;
	res->start_flag=1;
	res->after_decode=ad;
	return res;
}

int kuk_open(kuk_sock* ks, char *ip, uint32_t port){
	signal(SIGPIPE, SIG_IGN);    // sigpipe 무시.
	memset(&ks->sin,'\0',sizeof(ks->sin));
	ks->sin.sin_family=AF_INET;
	ks->sin.sin_port=htons(port);
	ks->sin.sin_addr.s_addr=inet_addr(ip);

	if((ks->fd=socket(AF_INET,SOCK_STREAM,0))==-1){
		fprintf(stderr,"sock error!\n");
		exit(1);
	}
	int option=1;
	setsockopt( ks->fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option) );
	return 1;
}
int kuk_bind(kuk_sock* ks){
	if(bind(ks->fd,(struct sockaddr*)&ks->sin,sizeof(ks->sin))){
		fprintf(stderr,"bind error!\n");
		exit(1);
	}
	return 1;
}
int kuk_listen(kuk_sock* ks,int num){
	if(listen(ks->fd,num)){
		fprintf(stderr,"listen error!\n");
		exit(1);
	}
	return 1;
}
int kuk_accept(kuk_sock* ks){
	int clnt_size=sizeof(ks->c_sin);
	if((ks->clnt=accept(ks->fd,(struct sockaddr*)&ks->c_sin,&clnt_size))==-1){
		fprintf(stderr,"accept error!");
		exit(1);
	}
	return 1;
}
int kuk_connect(kuk_sock *ks,char *ip, uint32_t port){
	memset(&ks->c_sin,0,sizeof(ks->c_sin));
	ks->c_sin.sin_family=AF_INET;
	ks->c_sin.sin_port=htons(port);
	ks->c_sin.sin_addr.s_addr=inet_addr(ip);

	if((ks->clnt=socket(AF_INET,SOCK_STREAM,0))==-1){
		fprintf(stderr,"sock error in clnt");
		exit(1);
	}

	if(connect(ks->clnt,(struct sockaddr*)&ks->c_sin,sizeof(ks->c_sin))){
		fprintf(stderr,"connect error!\n");
		exit(1);
	}
	return 1;
}
int kuk_send(kuk_sock* ks, char *buf, uint32_t size){
	return write(ks->clnt,buf,size);
}
int kuk_recv(kuk_sock* ks, char *buf, uint32_t size){
	return read(ks->clnt,buf,size);
}
int kuk_service(kuk_sock* ks){
	if(ks->decoder==NULL){
		return 0;
	}
	if(ks->start_flag ||(ks->disconnect_flag==0 && ks->data_idx==ks->data_size)){
		uint32_t len=0;
		char flag=0;
		ks->start_flag=0;
		while(len!=ks->data_size) {
			len+=kuk_recv(ks,&ks->data[len],ks->data_size-len);
			if(flag!=-1){
				//check for alive
				flag=kuk_send(ks,"a",strlen("a"));
			}
			else{
				ks->disconnect_flag=1;
				ks->data_lsize=len;
			}
			if(ks->data_lsize!=0 && len==ks->data_lsize){
				break;
			}
		}
		ks->data_idx=0;
	}
	else if(ks->disconnect_flag){
		
	}
	if(ks->decoder(ks,ks->after_decode)==NULL) return 0;
	return 1;
}

int kuk_request(kuk_sock *ks, int special_size){
	int len=0;
	int target_size;
	static int t_size=0;
	if(special_size){
		target_size=special_size;
	}
	else{
		target_size=ks->data_size;
	}
	
	while(len!=target_size){
		len+=kuk_send(ks,&ks->data[len],target_size-len);
	}
	if(special_size)
		return 0;
	kuk_recv(ks,ks->data,ks->data_size);
	return 1;
}

int kuk_sock_destroy(kuk_sock* ks){
	free(ks->data);
	if(ks->p_data){
		for(int i=0; ks->p_data[i]!=NULL; i++){
			free(ks->p_data[i]);
		}
	}
	if(ks->fd!=-1)
		close(ks->fd);
	if(ks->clnt!=-1)
		close(ks->clnt);
	free(ks);
	return 1;
}

int kuk_send_buf(kuk_sock *ks, char *t_data, int encoded_size, int special_size){

	if(!ks->start_flag && ks->data_idx==ks->data_size){
		kuk_request(ks,0);
		ks->data_idx=0;
	}else if(ks->start_flag){
		ks->start_flag=0;
		ks->data_idx=0;
	}
	memcpy(&ks->data[ks->data_idx],t_data,encoded_size);
	ks->data_idx+=encoded_size;
	if(special_size){
		kuk_request(ks,ks->data_idx);
	}
	return 1;
}

void *kuk_decoder_exp(kuk_sock *ks, void *(after_decode)(kuk_sock *)){
	static int parse_size=sizeof(uint8_t)+2*sizeof(uint32_t);
	char **parse=ks->p_data;
	if(parse==NULL){
		parse=(char**)malloc(3*sizeof(char*)+1);
		parse[0]=(char*)malloc(sizeof(uint8_t));//type
		parse[1]=(char*)malloc(sizeof(uint32_t));//key
		parse[2]=(char*)malloc(sizeof(uint32_t));//length
		parse[3]=NULL;
		ks->p_data=parse;
	}

	char *decode_data=&ks->data[ks->data_idx];
	memcpy(parse[0],&decode_data[0],sizeof(uint8_t));
	memcpy(parse[1],&decode_data[sizeof(uint8_t)],sizeof(uint32_t));
	memcpy(parse[2],&decode_data[sizeof(uint8_t)+sizeof(uint32_t)],sizeof(uint32_t));
	
	ks->data_idx+=parse_size;
	if(after_decode!=NULL){
		after_decode(ks);
	}
	if(*((int*)parse[0])==ENDFLAG){
		return NULL;
	}
	return (void*)parse;
}

/*
void encoding(char *data,uint8_t a, uint32_t b, uint32_t c){
	memcpy(&data[0],&a,sizeof(uint8_t));
	memcpy(&data[sizeof(uint8_t)],&b,sizeof(uint32_t));
	memcpy(&data[sizeof(uint8_t)+sizeof(uint32_t)],&c,sizeof(uint32_t));
}
#define PORT 8888
#define IP "127.0.0.1"
#define BSIZE 128
int main(int argc, char *argv[]){
	kuk_sock *test=kuk_sock_init(110*(sizeof(uint8_t)+2*sizeof(uint32_t)),kuk_decoder_exp,NULL);
	char buf[BSIZE];
	if(argv[1][0]=='s'){
		kuk_open(test,IP,PORT);
		kuk_bind(test);
		kuk_listen(test,5);
		kuk_accept(test);
		while(kuk_service(test)){
//		while(test->disconnect_flag==0){
			printf("%d - %u - %u\n",*((uint8_t*)test->p_data[0]),*((uint32_t*)test->p_data[1]),*((uint32_t*)test->p_data[2]));
		}
	}
	else{
		kuk_connect(test,IP,PORT);
		for(int i=0; i<100; i++){
			encoding(buf,1,i,4096);
			kuk_send_buf(test,buf,sizeof(uint32_t)*2+sizeof(uint8_t),0);
		}
		encoding(buf,ENDFLAG,0,0);
		kuk_send_buf(test,buf,sizeof(uint32_t)*2+sizeof(uint8_t),sizeof(uint32_t)*2+sizeof(uint8_t));
	}
	kuk_sock_destroy(test);
}*/

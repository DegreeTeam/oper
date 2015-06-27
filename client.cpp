#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define COMMU 0 
#define LOG 1 
#define PORT 9000
#define IPADDR "127.0.0.1"
#define PIADDR "192.168.42.1"
int flag = 1;
void* checkConnetion (void* tcp_socket){
	int* socket_addr = (int*)tcp_socket;
	int socket = *socket_addr;
	int ack = 1;
	while(1){
		sleep(2);		
//		fprintf(stderr, "push!!\n");
		if(send(socket, &ack, sizeof(ack), 0) < 0){
			fprintf(stderr, "server close!!\n");
			flag = 0;
			break;	
		}
	}

}
int main(){
	int tcp_socket;
        int c_socket;
        struct sockaddr_in c_addr;
        int len;    
	int udp_port;
	pthread_t pthread1;

        tcp_socket = socket(PF_INET, SOCK_STREAM, 0); 

        memset(&c_addr, 0, sizeof(c_addr));
        c_addr.sin_addr.s_addr = inet_addr(PIADDR);
        c_addr.sin_family = AF_INET;
        c_addr.sin_port = htons(PORT);

	if(connect(tcp_socket, (struct sockaddr*) &c_addr, sizeof(c_addr))==-1){
		printf("Can not connect\n");
		close(tcp_socket);
		return -1;	
	}
	
	int n;
	n = read(tcp_socket, &udp_port, sizeof(udp_port));
	if(n<=0){
		fprintf(stderr, "Can not receive port\n");
		close(tcp_socket);
		return 1;
	}
	printf("udp port = %d", udp_port);

	c_socket = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&c_addr, sizeof(c_addr));
	c_addr.sin_family = AF_INET;
	c_addr.sin_addr.s_addr = inet_addr(PIADDR);
	c_addr.sin_port = htons(udp_port);

	int ack = 1;
	char buff[512] = {0,};
	
//	struct timeval tv;
//	tv.tv_sec = 3;
//	tv.tv_usec = 0;
//	if (setsockopt(c_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
//	    perror("Error");
//	}
//	
	sleep(1);
	len = sizeof(c_addr);
	printf("before send udpPort = %d\n", udp_port);
#if !(COMMU)		
	while( (sendto(c_socket, (void *)&ack, sizeof(ack), 0,(struct sockaddr*)&c_addr , len)) <0 );
	printf("after send\n"); 
#endif
	int count = 0;

    	if(pthread_create(&pthread1 , NULL , checkConnetion , (void*) &tcp_socket ) < 0)
	{
           perror("could not create thread");
            return 1;
        }
	while(flag){
#if COMMU
		if(count == 0){
			while( (sendto(c_socket, (void *)&ack, sizeof(ack), 0,(struct sockaddr*)&c_addr , len)) <0 ); 
			count=100;
		}
#endif
		if( (recvfrom(c_socket, (void *)buff, sizeof(buff), 0, NULL, NULL)) <0 ){
			printf("receive error\n");
			break;
		}
//		count--;
#if LOG
		printf("port = %d \n", udp_port);
		for(int i=0; i<1;i++){
			fprintf(stderr,"%d ", buff[i]);
		}
		printf("\n\n");
#endif
	}

	pthread_join(pthread1, (void **) NULL);
	close(tcp_socket);
	close(c_socket);
	return 0;
}

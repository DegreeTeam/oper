#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define COMMU 1
#define LOG 0
#define PORT 9000
#define IPADDR "127.0.0.1"
#define PIADDR "192.168.42.1"
int main(){
        int c_socket;
        struct sockaddr_in c_addr;
        int len;    
	int udp_port;

        c_socket = socket(PF_INET, SOCK_STREAM, 0); 

        memset(&c_addr, 0, sizeof(c_addr));
        c_addr.sin_addr.s_addr = inet_addr(IPADDR);
        c_addr.sin_family = AF_INET;
        c_addr.sin_port = htons(PORT);

	if(connect(c_socket, (struct sockaddr*) &c_addr, sizeof(c_addr))==-1){
		printf("Can not connect\n");
		close(c_socket);
		return -1;	
	}
	
	int n;
	n = read(c_socket, &udp_port, sizeof(udp_port));
	if(n<=0){
		fprintf(stderr, "Can not receive port\n");
		close(c_socket);
		return 1;
	}
	printf("udp port = %d", udp_port);
	close(c_socket);

	c_socket = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&c_addr, sizeof(c_addr));
	c_addr.sin_family = AF_INET;
	c_addr.sin_addr.s_addr = inet_addr(IPADDR);
	c_addr.sin_port = htons(udp_port);

	int ack = 1;
	char buff[512] = {0,};
	
	struct timeval tv;
	tv.tv_sec = 4;
	tv.tv_usec = 0;
	if (setsockopt(c_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    perror("Error");
	}
	
	sleep(1);
	len = sizeof(c_addr);
#if !(COMMU)		
	while( (sendto(c_socket, (void *)&ack, sizeof(ack), 0,(struct sockaddr*)&c_addr , len)) <0 ); 
#endif
	while(1){
#if COMMU
		while( (sendto(c_socket, (void *)&ack, sizeof(ack), 0,(struct sockaddr*)&c_addr , len)) <0 ); 
#endif
		if( (recvfrom(c_socket, (void *)buff, sizeof(buff), 0, NULL, NULL)) <0 ){
			printf("receive error\n");
			break;
		}
#if LOG
		printf("port = %d \n", udp_port);
		for(int i=0; i<20;i++){
			printf("%d ", buff[i]);
		}
		printf("\n\n");
#endif
	}
	close(c_socket);
	return 0;
}

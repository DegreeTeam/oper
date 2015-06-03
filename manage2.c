#include <sys/types.h>
#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>

#define SIZE 32

int main(int argc , char *argv[])
{
    int socket_desc , client_sock, c, n;
    struct sockaddr_in server , client;
    char rcvBuffer[SIZE];
    pid_t pid;
    int bValid = 1;
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons( 2012 );

    //Create socket
    if ((socket_desc = socket(AF_INET , SOCK_STREAM , 0)) == -1)
    {
        printf("Could not create socket");
    }

    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR,&bValid,sizeof(bValid));
   //Bind
    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc , 3);
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while(1){

	  if(client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))
        { 
		printf("accept success \n");
	        while((n = read(client_sock, rcvBuffer, sizeof(rcvBuffer))))

			{
				int state;
				char str[SIZE] = {0,};
				int i;
				FILE *fp_in;

				 rcvBuffer[n] = '\0';
				 printf("%s\n", rcvBuffer);
				 fflush(stdout);
				state = rcvBuffer[0];
				
				for(i = 1; i<SIZE;i++){
					str[i-1] = rcvBuffer[i];
				}
	
				switch(state){
				case '1':
					if (fp_in = fopen("/home/pi/oper/name.config", "w")) {
					fprintf(fp_in, "%s", str);
					fclose(fp_in);
					}
				break;
				case '2':
					if (fp_in = fopen("/home/pi/oper/ssid.config", "w")) {
					fprintf(fp_in, "%s", str);
					fclose(fp_in);
					}	
				break;
				case '3':
					if (fp_in = fopen("/home/pi/oper/pw.config", "w")) {
					fprintf(fp_in, "%s", str);
					fclose(fp_in);
					}	
				break;
				case '4':
					if (fp_in = fopen("/home/pi/oper/ip.config", "w")) {
					fprintf(fp_in, "%s", str);
					fclose(fp_in);
					}	
				break;
				case '5':
					if (fp_in = fopen("/home/pi/oper/rssi.config", "w")) {
					fprintf(fp_in, "%s", str);
					fclose(fp_in);
                                        pid = fork();
                                        switch(pid)
                                        {
                                       	 case -1:
                                                printf("fork failed");
                                                break;
                                         case 0:
                                                execl("/home/pi/oper/setting", "setting", (char*) 0);
                                                printf("exec failed");
                                                break;
                                         default:
                                                wait((int*) 0);
                                                printf("ls completed\n");
                                         exit(0);
                                        }
				       }	
			         break;
			        }
			}
			close(client_sock);
        }
	}
	close(socket_desc);
    return 0;
}








#include <sys/types.h>
#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>    
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <alsa/asoundlib.h>
#include <time.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#define SIZE 512
#define PORT 9000
#define SERVERADDR "192.168.42.1"

#define PORTSIZE 50
#define LOCK 1
#define COMMU 0 
pthread_mutex_t mutex_lock   = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   thread_cond  = PTHREAD_COND_INITIALIZER;


void *data_streaming(void *socket_desc);
unsigned char* buffer;

typedef struct _TcpStatus{
	int socket;	
	int udp_socket;
	pthread_t thr_id;
	int flag;
} TcpStatus;

TcpStatus TCPCONN[PORTSIZE] = {0,};
int port[PORTSIZE] = { 0, };
int portP[PORTSIZE] = { 0, };
char user_num = 0;
void setPort(){
	for (int i = 0; i < PORTSIZE; i++){
		port[i] = 9001 + i;
		portP[i] = 0;
	}
}
int availablePort(){
	for (int i = 0; i < PORTSIZE; i++){
		if (portP[i] == 0){
			return i;
		}
	}
	return -1;
}
int _write(char* str){
	write(STDOUT_FILENO, str, strlen(str));
}
void sig_handler(int signo)
{
	//fprintf(stderr, "after disconnect User_num = %d\n", user_num);
	_write("remove do_echo get INT\n");
	exit(0);
}
void* do_echo(void* index){
	_write("Thread create!!!\n\n");
	int s_socket;
	struct sockaddr_in s_addr, c_addr;	
	int* addr = (int*) index;
	int INDEX = *addr;
	TCPCONN[INDEX].flag = 1;
	
	int len;
	int ack;
	s_socket = socket(PF_INET, SOCK_DGRAM, 0);

	delete(addr);	
	memset(&s_addr, 0, sizeof(s_addr));
        s_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(port[INDEX]);
	if(bind(s_socket, (struct sockaddr *) &s_addr, sizeof(s_addr)) == -1){
		_write("UDP bind error!\n");
		close(s_socket);
                return 0; 
        }	
	TCPCONN[INDEX].udp_socket = s_socket;
	int snd_buf = SIZE*2;
	if (setsockopt(s_socket, SOL_SOCKET, SO_RCVBUF,&snd_buf,sizeof(snd_buf)) < 0) {
	    perror("Error");
	}
	len = sizeof(c_addr);
	_write("before receive\n");

#if !(COMMU)
	if((recvfrom(s_socket, (void *)&ack, sizeof(ack), 0, (struct sockaddr *)&c_addr, (socklen_t*)&len)) <0 ){
		_write("recvfrom error\n");
		close(s_socket);
		return 0;
	}
	_write("connected\n");
#endif
	while(TCPCONN[INDEX].flag)
	{
#if LOCK
	        pthread_cond_wait(&thread_cond, &mutex_lock);
#endif
		while( (sendto(s_socket, (void *)buffer, SIZE, 0, (struct sockaddr *)&c_addr, len)) <0 );
	}
	close(s_socket);
	return 0;
}

int getMaxfd(){
	int max = 0;
	for(int i=0;i<PORTSIZE;i++){
		if(TCPCONN[i].socket > max){
			max = TCPCONN[i].socket;
		}
	}
	return max;
}

void* check_conn(void* param){
	int maxfd;	
	struct timeval timeout = {0,};
	timeout.tv_usec = 1;
	fd_set read_fds;
	int ack;
	while(1){
		maxfd = getMaxfd() +1;
		FD_ZERO(&read_fds);
		for(int i=0;i<PORTSIZE;i++){
			if(TCPCONN[i].socket >0){
				FD_SET(TCPCONN[i].socket, &read_fds);
			}
		}
		if(select(maxfd, &read_fds, NULL, NULL, &timeout)<=0){
			continue;
		}
		for(int i=0;i<PORTSIZE;i++){
			if(TCPCONN[i].socket >0){
				if(FD_ISSET(TCPCONN[i].socket, &read_fds)){
					if(read(TCPCONN[i].socket, &ack, sizeof(ack))<=0){
						_write("Check_conn --Close Connection\n");
						user_num--;
						portP[i] = 0;
						close(TCPCONN[i].socket);
						close(TCPCONN[i].udp_socket);
						TCPCONN[i] = {0,};
					}
				}
			}
		}
	}

	return 0;
}
int main(){
	int c_socket, s_socket;
	struct sockaddr_in s_addr, c_addr;
	int len;
	pthread_t pthread1, pthread2, pthread3;
	int thr_id;
	int status;

	setPort();	
	s_socket = socket(PF_INET, SOCK_STREAM, 0); 

        memset(&s_addr, 0, sizeof(s_addr));
        s_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(PORT);
	
	int optval = 1;
	int optlen = sizeof(int);
	if(setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, optlen)<0){
                printf("reuse option fail\n");
	}
        if(bind(s_socket, (struct sockaddr *) &s_addr, sizeof(s_addr)) == -1){
                printf("Can not Bind\n");
                return -1; 
        }
    
        if(listen(s_socket, 50) == -1){
                printf("listen Fail\n");
                return -1; 
        }

	buffer = (unsigned char *) malloc(SIZE);
	for(int i=0; i<SIZE; i++){
		buffer[i] = 0;
	}

    	if(pthread_create(&pthread2 , NULL , data_streaming , (void*) NULL) < 0)
	{
           perror("could not create thread");
            return 1;
        }
    	if(pthread_create(&pthread3 , NULL , check_conn , (void*) NULL) < 0)
	{
           perror("could not create thread");
            return 1;
        }
	
	fd_set read_fds;
	while(1){
		int num;
		num = availablePort();
		
		if(num != -1){
			len = sizeof(c_addr);
			_write("before accept\n");
			c_socket = accept(s_socket, (struct sockaddr *) &c_addr, (socklen_t*)&len);
			write(c_socket, &port[num], sizeof(port[num]));
			
			int* addr = new int;
			*addr = num;

			thr_id = pthread_create(&pthread1, NULL, do_echo, (void*) addr);
			pthread_detach(pthread1);
			fprintf(stderr, "thread create~~id = %d\n", thr_id);

			portP[num] = 1;
			user_num++;
			fprintf(stderr, "user number = %d\n", user_num);
			TcpStatus tcp;
			tcp.socket = c_socket;
			tcp.thr_id = pthread1;
			tcp.flag = 1;
			TCPCONN[num] = tcp;
		}
	}
	pthread_join(pthread2, (void **) status);
	close(s_socket);
	return 0;
}
void *data_streaming(void *socket_desc)
{
	int rc;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	int i;

	/* Open PCM device for recording (capture). */
	rc = snd_pcm_open(&handle, "plughw:1,0",SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr,"unable to open pcm device: %s\n",
		snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_U8);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 1);

	/* 44100 bits/second sampling rate (CD quality) */
	val = 40960;
	snd_pcm_hw_params_set_rate_near(handle, params,&val, &dir);

	/* Set period size to 32 frames. */
	frames = SIZE;
	snd_pcm_hw_params_set_period_size_near(handle,params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n",
		snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params,&frames, &dir);
	/* 2 bytes/sample, 2 channels */

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params,&val, &dir);

	while (1) {
#if LOCK
//		pthread_mutex_lock(&mutex_lock);
#endif
		rc = snd_pcm_readi(handle, buffer, SIZE);
#if LOCK
        	pthread_cond_broadcast(&thread_cond);
//	        pthread_mutex_unlock(&mutex_lock);
#endif
		if (rc == -EPIPE) {
		/* EPIPE means overrun */
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr,"error from read: %s\n",snd_strerror(rc));
		} else if (rc != (int)frames) {
			fprintf(stderr, "short read, read %d frames\n", rc);
		}
	}
	free(buffer);
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	return 0;
}

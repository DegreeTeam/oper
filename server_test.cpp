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
#define SIZE 2048
#define PORT 9000
#define SERVERADDR "192.168.42.1"

#define PORTSIZE 50
#define LOCK 1
#define COMMU 0 
pthread_mutex_t mutex_lock   = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   thread_cond  = PTHREAD_COND_INITIALIZER;


void *data_streaming(void *socket_desc);
unsigned char* buffer;

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
typedef struct _UdpStatus{
	int num;	
	struct sockaddr_in addr;
} UdpStatus;
void* do_echo(void* index){
	int s_socket;
	struct sockaddr_in s_addr, c_addr;	
	UdpStatus* setting = (UdpStatus*) index;
	int len;
	int ack;
	c_addr = setting->addr;
	s_socket = socket(PF_INET, SOCK_DGRAM, 0);
	
	memset(&s_addr, 0, sizeof(s_addr));
        s_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(port[setting->num]);
	if(bind(s_socket, (struct sockaddr *) &s_addr, sizeof(s_addr)) == -1){
		close(s_socket);
                return 0; 
        }	
	
	struct timeval tv;
	tv.tv_sec = 100;
	tv.tv_usec = 0;
	if (setsockopt(s_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    perror("Error");
	}
	int snd_buf = SIZE*2;
	if (setsockopt(s_socket, SOL_SOCKET, SO_RCVBUF,&snd_buf,sizeof(snd_buf)) < 0) {
	    perror("Error");
	}
	c_addr = setting->addr;
	len = sizeof(c_addr);
	_write("before receive\n");
#if !(COMMU)
	if((recvfrom(s_socket, (void *)&ack, sizeof(ack), 0, (struct sockaddr *)&c_addr, (socklen_t*)&len)) <0 ){
		_write("recvfrom error\n");
		portP[setting->num] = 0;
		user_num--;
		delete(setting);
		close(s_socket);
		return 0;
	}
	_write("connected\n");
#endif
	int count = 0;
	while(1)
	{
#if COMMU
		if((recvfrom(s_socket, (void *)&ack, sizeof(ack), 0, (struct sockaddr *)&c_addr, (socklen_t*)&len)) <0 ){
			_write("recvfrom error\n");
			break;
		}	
#endif
#if LOCK
//		pthread_mutex_lock(&mutex_lock);
	        pthread_cond_wait(&thread_cond, &mutex_lock);
#endif
		while( (sendto(s_socket, (void *)buffer, SIZE, 0, (struct sockaddr *)&c_addr, len)) <0 );
		count++;
		if(count==100){
			if((recvfrom(s_socket, (void *)&ack, sizeof(ack), 0, (struct sockaddr *)&c_addr, (socklen_t*)&len)) <0 ){
				_write("recvfrom error\n");
				break;
			}	
			count = 0;
		}
#if LOCK
//		pthread_mutex_unlock(&mutex_lock);
#endif

	}
	portP[setting->num] = 0;
	user_num--;
	delete(setting);
	close(s_socket);
	return 0;
}
int main(){
	int c_socket, s_socket;
	struct sockaddr_in s_addr, c_addr;
	int len;
	pthread_t pthread1, pthread2;
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

	while(1){
		int num;
		if ((num = availablePort()) != -1){
			fprintf(stderr, "Port is available!\n");
			portP[num] = 1;
			len = sizeof(c_addr);
			c_socket = accept(s_socket, (struct sockaddr *) &c_addr, (socklen_t*)&len);
			write(c_socket, &port[num], sizeof(port[num]));
			close(c_socket);
			
			user_num++;
			fprintf(stderr, "user number = %d\n", user_num);
			UdpStatus* setting = new UdpStatus;

			setting->num = num;
			setting->addr = c_addr;
			thr_id = pthread_create(&pthread1, NULL, do_echo, (void*) setting);
			pthread_detach(pthread1);
		}
		else{
			printf("There are no available port!\n");
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
//		if (rc == -EPIPE) {
//		/* EPIPE means overrun */
//			fprintf(stderr, "overrun occurred\n");
//			snd_pcm_prepare(handle);
//		} else if (rc < 0) {
//		//	fprintf(stderr,"error from read: %s\n",snd_strerror(rc));
//		} else if (rc != (int)frames) {
//		//	fprintf(stderr, "short read, read %d frames\n", rc);
//		}
	}
	free(buffer);
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	return 0;
}

#include<sys/types.h>
#include <sys/types.h>
#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <alsa/asoundlib.h>
#include <time.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#define SIZE 64 

void *data_sending(void *);
void data_sending_handler();
void *data_streaming(void *);
void closesock(int *sock, int index);
void int_handler();

pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned char *buffer;
int **sockid;
int *delsock;
int cnt = 0;
int delcnt = 0;
int tid;
int socket_desc;
int flag;

int main(int argc , char *argv[])
{
    int client_sock , c , *new_sock;
    struct sockaddr_in server , client;
    pthread_t sniffer_thread1, sniffer_thread2;
    int bValid = 1;
    struct sigaction usr;

    flag =1;
    memset(&usr,0,sizeof(struct sigaction));
    usr.sa_handler = int_handler;
    sigemptyset(&usr.sa_mask);
    sigaction(SIGINT, &usr, NULL);
    buffer = (unsigned char *) malloc(SIZE);
    sockid = (int**) malloc(sizeof(int*) * 40);
   memset(sockid,0 , sizeof(int*) * 40);
    delsock = (int*)malloc(sizeof(int)*20);
    memset(delsock, 0, sizeof(int)*20);
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons( 2008 );


    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR,&bValid,sizeof(bValid));

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
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

    if(pthread_create(&sniffer_thread1 , NULL , data_streaming , (void*) new_sock) < 0)
       {
            perror("could not create thread");
            return 1;
        }
	
    if(pthread_create(&sniffer_thread2 , NULL , data_sending , (void*) new_sock) < 0)
       {
            perror("could not create thread");
            return 1;
        }
	tid = sniffer_thread2;
    while(1){
	 if(client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))
	  {
		puts("Connection accepted");
                new_sock = malloc(4);
                *new_sock = client_sock;
        }
	   if (cnt < 40)
		   	 sockid[cnt++] = new_sock;
	   else
		   	sockid[delsock[delcnt--]] = new_sock;

       puts("Handler assigned");

    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
}
    return 0;
}
void *data_sending(void *socket_desc)
{
	struct sigaction sa_usr;
	memset(&sa_usr,0,sizeof(struct sigaction));
	sa_usr.sa_handler = data_sending_handler;
	sigemptyset(&sa_usr.sa_mask);
	sigaction(SIGUSR1, &sa_usr, NULL);
	
	while(1)
		pause();
}
void data_sending_handler()
{
	int i;
      	for(i = 0; i < cnt;i++) {
           if ( *(sockid[i]) != -1 && (send(*(sockid[i]), buffer, SIZE, 0) < 0 || signal(SIGPIPE, SIG_IGN) == SIG_ERR )   ) {
               closesock(sockid[i],i);
               printf("close sock\n : %d",*sockid[i]);
            }
	}
}
void closesock(int *sock,int index)
{
	delsock[delcnt++] = index;
	close(*sock);
	*sock = -1;
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
  rc = snd_pcm_open(&handle, "plughw:1,0",
                    SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_U8);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 1);

  /* 44100 bits/second sampling rate (CD quality) */
  val = 32768;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);

  /* Set period size to 32 frames. */
  frames = SIZE;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params,
                                      &frames, &dir);
 /* 2 bytes/sample, 2 channels */

  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(params,
                                         &val, &dir);
  
  while (1) {
        pthread_mutex_lock(&mutex);
        rc = snd_pcm_readi(handle, buffer, frames);
        pthread_kill(tid, SIGUSR1);
        pthread_mutex_unlock(&mutex);

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

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

    return 0;
}

void int_handler()
{
	int i;
		for (i = 0; i < cnt; i++) {
           if ( *(sockid[i]) != -1 ) {
               close(*sockid[i]);
            }
     }
	close(socket_desc);
	free(sockid);
	exit(-1);
}


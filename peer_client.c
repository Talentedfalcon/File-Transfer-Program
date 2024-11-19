#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "message.h"
#include <sys/time.h>
#include <semaphore.h>
#include <signal.h>

#define BUF_SIZE 1024

int clientfd;

int not_sending=0;

sem_t mutex;

void receive_file(char* filepath){
    char buffer[BUF_SIZE];
    Message m;
    size_t recv_status;

    FILE* fptr=fopen(filepath,"wb");

    while(1){
        memset(buffer,0,BUF_SIZE);
        recv_status=recv(clientfd,&m,sizeof(Message),0);
        printf("%d {%s}",m.status,m.msg);
        printf("\n\n");
        if(recv_status<0){
            perror("Peer not found");
        }
        if(m.status==FILE_TRANSFER){
            fwrite(m.msg,1,strlen(m.msg),fptr);
        }
        else if(m.status==END_OF_FILE){
            break;
        }
        sleep(1);
    }
    fclose(fptr);
}

void send_file(char* filepath){
    char buffer[BUF_SIZE];
    Message m;
    size_t num_read;
    
    FILE* fptr=fopen(filepath,"rb");

    while(1){
        memset(buffer,0,BUF_SIZE);
        num_read=fread(buffer,sizeof(char),BUF_SIZE,fptr);
        if(ferror(fptr)){
            perror("Error in reading file...\n");
            break;
        }
        m=*init_msg(FILE_TRANSFER,buffer);
        printf("%d {%s}",m.status,m.msg);
        printf("\n\n");
        size_t send_status=send(clientfd,&m,sizeof(m),0);
        if(send_status<0){
            perror("ERROR writing to buffer");
        }
        if(num_read<BUF_SIZE){
            break;
        }
    }

    m=*init_msg(END_OF_FILE,"");
    send(clientfd,&m,sizeof(Message),0);

    fclose(fptr);
}

void receiving_segment(){
    Message m;
    while(1){
        sem_wait(&mutex);
        struct timeval timeout={3,0};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(clientfd,&fds);
        int ret=select(clientfd+1,&fds,NULL,NULL,&timeout);
        if(ret>0){
            int recv_status=recv(clientfd,&m,sizeof(Message),0);
            if(recv_status<0){
                perror("Server not found");
                pthread_exit(NULL);
            }
            else if(m.status==SEND_FILE_REQUEST){
                char* filepath=(char*)malloc((strlen(m.msg)+1)*sizeof(char));
                strcpy(filepath,m.msg);
                printf("Peer want to send %s\n",filepath);
                char option[10];
                printf("Do you want to receive %s? [y/n]\n",filepath);
                fgets(option,10,stdin);
                if(option[0]=='y'){
                    m=*init_msg(ACK_SEND_FILE,"");
                    printf("Sending ACK...\n");
                    send(clientfd,&m,sizeof(Message),0);
                    receive_file(filepath);
                }
                else{
                    printf("No File for me...\n");
                }
                not_sending=0;
            }
        }
        sem_post(&mutex);
        sleep(1);
    }
}

void sending_segment(){
    Message m;
    while(1){
        sem_wait(&mutex);
        if(!not_sending){
            char option[10];
            printf("\nDo you want to send to the client? [y/n]\n");
            printf("Option:\n");
            fgets(option,10,stdin);
            if(option[0]=='y'){
                FILE* fptr;
                printf("Input filepath:\n");
                char* filepath=(char*)malloc(1024*sizeof(char));
                fgets(filepath,1024,stdin);
                filepath[strlen(filepath)-1]='\0';

                fptr=fopen(filepath,"r");

                if(fptr==NULL){
                    perror("File does not exists");
                }
                else{
                    fclose(fptr);
                    m=*init_msg(SEND_FILE_REQUEST,filepath);
                    printf("Sending %s\n",m.msg);
                    send(clientfd,&m,sizeof(Message),0);
                    recv(clientfd,&m,sizeof(Message),0);
                    printf("ACK received...\n");
                    send_file(filepath);
                }
            }
            else{
                printf("Not Sending...\n");
                not_sending=1;
            }
        }
        sem_post(&mutex);
        sleep(1);
    }
}

void handle_SIGINT(){
    close(clientfd);
    signal(SIGINT,SIG_DFL);
    raise(SIGINT);
}

int main(int argc, char **argv) {
    printf("\033[H\033[J");
    // printf("%d\n",argc);
    // for(int i=0;i<argc;i++){
    //     printf("%s\n",argv[i]);
    // }

    signal(SIGINT,handle_SIGINT);

    char *server_ip = argv[1];
    int PORT = atoi(argv[2]);

    char buffer[BUF_SIZE];
    struct sockaddr_in server_addr;
    int server_addrlen=sizeof(server_addr);

    clientfd=socket(AF_INET,SOCK_STREAM,0);
    if(clientfd<0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);

    if(inet_pton(AF_INET,server_ip,&server_addr.sin_addr)<=0){
        perror("inet_pton failed");
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    while(connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Connect failed");
        sleep(1);
    }
    printf("Connected with server...\n");

    pthread_t recv_th,send_th;
    sem_init(&mutex,0,1);

    pthread_create(&recv_th,NULL,(void *)&receiving_segment,NULL);
    pthread_create(&send_th,NULL,(void *)&sending_segment,NULL);

    pthread_join(recv_th,NULL);
    pthread_join(send_th,NULL);

    sem_destroy(&mutex);

    return 0;
}

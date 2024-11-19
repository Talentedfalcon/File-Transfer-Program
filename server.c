#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "message.h"
#include <semaphore.h>

#define PORT 12000
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 3

sem_t mutex;
sem_t sendIP;
// sem_t recvIP;

int serverfd;
int connect_sockets[MAX_CONNECTIONS];
pthread_t thid[MAX_CONNECTIONS];
int num_connected=0;
char** IP_Addrs;
int num_IP_Addrs=0;
char buffer[BUFFER_SIZE];



void reset_all_sockets(){   
    for(int i=0;i<MAX_CONNECTIONS;i++){
        connect_sockets[i]=-1;
    }
}

void reset_all_threads(){
    for(int i=0;i<MAX_CONNECTIONS;i++){
        thid[i]=-1;
    }
}

void handle_SIGINT(){
    printf("Closing Server...\n");
    for(int i=0;i<MAX_CONNECTIONS;i++){
        if(connect_sockets[i]!=-1){
            close(connect_sockets[i]);
        }
    }
    close(serverfd);
    exit(0);
}

void handle_SIGPIPE(){
    
}

int updated=0;
void send_IP_list(){
    char* temp=(char*)malloc(SIZE_MSG*sizeof(char));
    while(1){
        if(num_IP_Addrs>0 && updated){
            sem_wait(&mutex);
            printf("Sending IP List to clients...\n");
            
            int len_temp=0;
            Message IP_list_m;

            for(int i=0;i<MAX_CONNECTIONS;i++){
                if(IP_Addrs[i]!=NULL){
                    for(int j=0;j<strlen(IP_Addrs[i]);j++){
                        temp[len_temp++]=IP_Addrs[i][j];
                    }
                    // temp[len_temp++]=i;
                    temp[len_temp++]='\n';
                }
            }
            temp[len_temp++]=64;
            printf("%s\tLength: %d\n",temp,strlen(temp));
            for(int i=0;i<MAX_CONNECTIONS;i++){
                if(IP_Addrs[i]!=NULL){
                    temp[len_temp++]=i+1;
                    temp[len_temp++]=' ';
                }
            }
            temp[len_temp]='\0';

            IP_list_m=*init_msg(RECIEVE_IP,temp);
            for(int i=0;i<MAX_CONNECTIONS;i++){
                if(IP_Addrs[i]!=NULL){
                    send(connect_sockets[i],&IP_list_m,sizeof(Message),0);
                }
            }
            updated=0;
            sem_post(&mutex);

            sleep(4);
        }
    }
}

void client_session(void* _client_id){
    // signal(SIGPIPE,handle_SIGPIPE);
    int* temp=(int *)_client_id;
    int client_id=*temp;
    Message th_m;
    
    printf("In session %d\n",client_id);
    while(connect_sockets[client_id]>=0){
        sleep(1);
        int recv_status=recv(connect_sockets[client_id],&th_m,sizeof(Message),0);
        if(recv_status<0){
            perror("Client not responding");
            break;
        }
        sleep(1);
        if(th_m.status==APPEND_IP){
            sem_wait(&mutex);
            // printf("Client %d: %s\n",client_id,th_m.msg);
            char* IP=(char*)malloc(100*sizeof(char));
            strcpy(IP,th_m.msg);
            
            IP_Addrs[client_id]=IP;
            num_IP_Addrs++;
            updated=1;
            sem_post(&sendIP);

            printf("Recieved IP from client %d: %s\n",client_id,IP_Addrs[client_id]);
            sem_post(&mutex);
        }
        else if(th_m.status==REQUEST_CLIENT_CONNECTION){
            printf("Requesting client connection...\n");
            int req_client_id=atoi(th_m.msg);
            if(connect_sockets[req_client_id]>=0){
                char* temp=(char*)malloc(20*sizeof(char));
                snprintf(temp,20,"%d",client_id);
                th_m=*init_msg(REQUEST_CLIENT_CONNECTION,temp);
                send(connect_sockets[req_client_id],&th_m,sizeof(Message),0);
                free(temp);
            }
            else{
                th_m=*init_msg(ERROR,"");
                send(connect_sockets[client_id],&th_m,sizeof(Message),0);
            }
        }
        else if(th_m.status==ACKNOWLEDGE_CLIENT_CONNECTION){
            printf("%s\n",th_m.msg);
            //Fill out this segment

            //send(connect_sockets[client_id],&th_m,sizeof(Message),0);
        }
        else if(th_m.status==BREAK_CONNECTION){
            // free(IP_Addrs[client_id]);
            IP_Addrs[client_id]=NULL;
            num_IP_Addrs--;
            updated=1;
            break;
        }
    }
    printf("Client %d Disconnected...\n",client_id);
    close(connect_sockets[client_id]);
    connect_sockets[client_id]=-1;
    num_connected--;
}

int find_available_socket(){
    for(int i=0;i<MAX_CONNECTIONS;i++){
        if(connect_sockets[i]==-1){
            return i;
        }
    }
    return -1;
}

int main(){
    signal(SIGINT,handle_SIGINT);

    sem_init(&mutex,0,1);
    sem_init(&sendIP,0,0);
    IP_Addrs=(char**)malloc(MAX_CONNECTIONS*sizeof(char*));
    reset_all_sockets();
    reset_all_threads();

    int catch_socket;
    Message m;

    struct sockaddr_in addr;
    int addrlen=sizeof(addr);

    serverfd=socket(AF_INET,SOCK_STREAM,0);
    if(serverfd<0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt=1;
    setsockopt(serverfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(PORT);

    if(bind(serverfd,(struct sockaddr*)&addr,sizeof(addr))<0){
        perror("Bind failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }

    pthread_t IP_list_th;
    pthread_create(&IP_list_th,NULL,(void*)&send_IP_list,NULL);

    while(1){
        printf("Server is listening...\n");
        if(listen(serverfd,MAX_CONNECTIONS)){
            perror("Listen failed");
            close(serverfd);
            exit(EXIT_FAILURE);
        }

        if((catch_socket=accept(serverfd,(struct sockaddr*)&addr,&addrlen))<0){
            perror("Accept failed");
            close(serverfd);
            exit(EXIT_FAILURE);
        }
        else{
            printf("Client connection accepted...\n");
            printf("Creating client session...\n");
            int available=find_available_socket();
            if(available==-1){
                m=*init_msg(MAX_CAP_MSG,"Max connections reached");
                send(catch_socket,&m,sizeof(Message),0);
                continue;
            }
            connect_sockets[available]=catch_socket;
            num_connected++;
            pthread_create(&(thid[available]),NULL,(void*)&client_session,(void *)&available);
        }
    }

    pthread_join(IP_list_th,NULL);
    for(int i=0;i<=MAX_CONNECTIONS;i++){
        if(thid[i]!=-1){
            pthread_join(thid[i],NULL);
        }
    }

    close(serverfd);
    printf("Server Exit...\n");
    return 0;
}
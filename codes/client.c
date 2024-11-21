#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include "message.h"
#include <netdb.h>
#include <pthread.h>
#include <sys/select.h>
#include <semaphore.h>

#define PORT 12000
#define BUFFER_SIZE 1024

sem_t mutex;

int serverfd;
int clientfd;
char** IP_Addrs;
int* client_ids;
int option=-1;
int num_IP_Addrs=0;
char buffer[BUFFER_SIZE];
Message m;

int connect_req=0;

void disconnect_from_server(){
    m=*init_msg(BREAK_CONNECTION,"");
    send(serverfd,&m,sizeof(Message),0);
    sleep(1);
    close(serverfd);
}

void handle_SIGINT(){
    disconnect_from_server();
    exit(EXIT_SUCCESS);
}

void handle_SIGPIPE(){
    close(serverfd);
    exit(EXIT_FAILURE);
}

char* get_IP_addr(){
    char hostbuffer[256];
    char* IPbuffer;
    struct hostent *host_entry;
    int hostname;

    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    host_entry = gethostbyname(hostbuffer);
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));

    return IPbuffer;
}

void breakdown_msg(char* msg){
    int offset=0;

    free(IP_Addrs);
    IP_Addrs=(char**)malloc(100*sizeof(char*));
    num_IP_Addrs=0;
    char* temp=(char*)malloc(20*sizeof(char));
    int len_temp=0;
    for(int i=0;msg[i]!=64;i++){
        offset++;
        if(msg[i]=='\n'){
            temp[len_temp]='\0';
            IP_Addrs[num_IP_Addrs++]=temp;
            temp=(char*)malloc(20*sizeof(char));
            len_temp=0;
            continue;
        }
        temp[len_temp++]=msg[i];
    }

    client_ids=(int*)malloc(100*sizeof(int));
    for(int i=0;i<2*num_IP_Addrs;i++){
        if((i%2)==0){
            client_ids[(i/2)]=msg[offset+1+i]-1;
        }
    }
}

void sending_segment(){
    printf("Sending to device IP to Server...\n");
    char* device_IP=get_IP_addr();
    m=*init_msg(APPEND_IP,device_IP);
    int send_status=send(serverfd,&m,sizeof(Message),0);
    if (send_status<0){
        perror("Server not found");
        exit(EXIT_FAILURE);
    }
}

void receiving_segment(){
    while(1){
        int recv_status=recv(serverfd,&m,sizeof(Message),0);
        if(recv_status<0){
            // perror("Server not found");
            pthread_exit(NULL);
        }
        else if(m.status==RECIEVE_IP){
            sleep(1);
            sem_wait(&mutex);
            breakdown_msg(m.msg);
            sem_post(&mutex);
        }
        else if(m.status==REQUEST_CLIENT_CONNECTION){
            connect_req=1;
            
            int TRANSFER_PORT=12001;
            struct sockaddr_in temp_addr;

            int tempfd=socket(AF_INET,SOCK_STREAM,0);
            temp_addr.sin_family=AF_INET;
            temp_addr.sin_addr.s_addr=INADDR_ANY;
            temp_addr.sin_port=htons(TRANSFER_PORT);

            while(bind(tempfd,(struct sockaddr*)&temp_addr,sizeof(temp_addr))<0){
                temp_addr.sin_port=htons(++TRANSFER_PORT);
            }
            printf("Sending connection acknowledgement along with TRANSFER_PORT...\n");
            char* temp=(char*)malloc(20*sizeof(char));
            snprintf(temp,20,"%d",TRANSFER_PORT);
            m=*init_msg(ACKNOWLEDGE_CLIENT_CONNECTION,temp);
            send(serverfd,&m,sizeof(Message),0);
            sleep(1);
            close(tempfd);
            disconnect_from_server();
            sleep(1);
            execl("./_peer_server","./_peer_server",temp);
            exit(EXIT_SUCCESS);
        }
        else if(m.status==ACKNOWLEDGE_CLIENT_CONNECTION){
            char* transfer_port=(char*)malloc((strlen(m.msg)+1)*sizeof(char));
            strcpy(transfer_port,m.msg);
            printf("Received acknowledgement from client %d \n\tTRANSFER_PORT=%s\n",client_ids[option-1],transfer_port);
            disconnect_from_server();
            sleep(1);
            execl("./_peer_client","./_peer_client",IP_Addrs[option-1],transfer_port);
            exit(EXIT_SUCCESS);
        }
        else if(m.status==MAX_CAP_MSG){
            printf("%s\n",m.msg);
            break;
        }
        else if(m.status==ERROR){
            printf("Error occurred...\n");
            break;
        }
        sleep(1);
    }
    raise(SIGINT);
}

void input_segment(){
    while(option<=0 && !connect_req){
        sem_wait(&mutex);
        if(num_IP_Addrs>0){
            option=-1;
            printf("\033[H\033[J");
            for(int i=0;i<num_IP_Addrs;i++){
                printf("%d. Client %d\n",i+1,client_ids[i]);
            }
            struct timeval timeout = {3, 0};
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            printf("Option:\n");
            int ret = select(1, &fds, NULL, NULL, &timeout);
            if(ret == -1){
                printf("Oops! Something wrong happened...\n");
            }
            else if(ret==0){
                // printf("Input timeout\n");
            }
            else{
                char name[10];
                fgets(name, 10, stdin);
                option=atoi(name);
            }
        }
        sem_post(&mutex);
        sleep(1);
    }

    if(option!=-1){
        printf("Requesting connection with client %d...\n",client_ids[option-1]);
        Message temp_m;
        char* temp=(char*)malloc(20*sizeof(char));
        sprintf(temp,"%d",client_ids[option-1]);
        temp_m=*init_msg(REQUEST_CLIENT_CONNECTION,temp);
        send(serverfd,&temp_m,sizeof(Message),0);
    }
}

int main(int argc,char** argv){
    if(argc!=3){
        printf("Invalid Arguements\n");
        printf("Usage:\n\t %s <ip-address> <port-number>",argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT,handle_SIGINT);
    signal(SIGPIPE,handle_SIGPIPE);

    sem_init(&mutex,0,1);

    struct sockaddr_in server_addr;
    int server_addrlen=sizeof(server_addr);

    serverfd=socket(AF_INET,SOCK_STREAM,0);
    if(serverfd<0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(atoi(argv[2]));

    if(inet_pton(AF_INET,argv[1],&server_addr.sin_addr)<=0){
        perror("inet_pton failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }

    if(connect(serverfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Connect failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }

    pthread_t recv_th,send_th,input_th;

    pthread_create(&recv_th,NULL,(void*)&receiving_segment,NULL);
    pthread_create(&send_th,NULL,(void*)&sending_segment,NULL);
    pthread_create(&input_th,NULL,(void*)&input_segment,NULL);

    pthread_join(send_th,NULL);
    pthread_join(recv_th,NULL);
    pthread_join(input_th,NULL);

    printf("Hello there\n");

    sem_close(&mutex);
    close(serverfd);
    return 0;
}
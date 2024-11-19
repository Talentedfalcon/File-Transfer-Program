#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_MSG 2048

#define ERROR -2
#define MAX_CAP_MSG -1

#define GENERAL_MSG 0

#define APPEND_IP 1
#define RECIEVE_IP 2

#define REQUEST_CLIENT_CONNECTION 3
#define ACKNOWLEDGE_CLIENT_CONNECTION 4

#define SEND_FILE_REQUEST 5
#define ACK_SEND_FILE 6

#define FILE_TRANSFER 7
#define END_OF_FILE 8

#define BREAK_CONNECTION 100

typedef struct{
    int status;
    char msg[SIZE_MSG];
}Message;

Message* init_msg(int status,char* msg){
    Message* m=(Message*)malloc(sizeof(Message));
    m->status=status;
    strcpy(m->msg,msg);
    return m;
}
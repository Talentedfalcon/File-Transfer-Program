#include <stdio.h>

int main(int argc,char** argv){
    printf("\033[H\033[J");
    printf("%d\n",argc);
    for(int i=0;i<argc;i++){
        printf("%s\n",argv[i]);
    }
}
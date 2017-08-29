#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<stdio.h>
int main(){
    char* argv[]={"vim","Main.c",0};
    pid_t pid=fork();
    if(pid==0){
        execvp(argv[0],argv);
        exit(0);
    }
    wait();
    return 0;
}


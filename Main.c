#include<stdio.h>
#include<time.h>
#include<pwd.h>
#include<grp.h>
#include<string.h>
#include<inttypes.h>
#include<stdlib.h>
#include<unistd.h>
#include<limits.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>

// MacOS definitions 
//#define MAC_OS
#ifdef MAC_OS
#define HOST_NAME_MAX 64
#define LOGIN_NAME_MAX 256
#endif
// End of MacOS definitions
//
// Global variables
char prevdir[PATH_MAX+1];
// End of global variables
void pinfo(char* argv[]){
    if(argv[1]==NULL){
        
        pid_t pid=getpid();
        char spid[50];
        sprintf(spid,"%d",pid);
        char status[10]="/status";
        char prox[100]="/proc/";
        strcat(spid,status);
        strcat(prox,spid);
       
        char *final[]={"cat",prox,0};
        pid_t pid_fork=fork();
        if(pid_fork==0){
            
            execvp(final[0],final);
            exit(0);
        }
        else{
            wait(NULL);
        }
        char proc[200]="/proc/";
        char nspid[100];
        sprintf(nspid,"%d",pid);
        strcat(proc,nspid);
        char exe[100]="/exe";
        strcat(proc,exe);
        pid_t nfork=fork();
        if(nfork==0){
            char* get_exe[]={"readlink",proc,0};
            execvp(get_exe[0],get_exe);
            exit(0);
        }
        else{
            wait(NULL);
        }


        
    }
    else{
        char proc[100]="/proc/";
        strcat(proc,argv[1]);
        char exe[200];
        strcpy(exe,proc);
        strcat(exe,"/exe");
        char status[50]="/status";
        strcat(proc,status);
        char * final[]={"cat",proc,0};
        pid_t pid_fork=fork();
        if(pid_fork==0){
            execvp(final[0],final);
        }
        else{
            wait(NULL);
        }
        pid_t nfork=fork();
        if(nfork==0)
        {
            char* get_exe[]={"readlink",exe,0};
            execvp(get_exe[0],get_exe);
            exit(0);
        }
        else{
            wait(NULL);
        }
        //char *final[]={"cat","/proc/[pid]/status",0}
    }
}
void runProcessBackground(char* argv[]){
    pid_t pid=fork();
    if(pid==0)
    {
        if(execvp(argv[0],argv)<0){
            printf("Command not found\n");
        }
        exit(0);
    }
    else{
        printf("pid:%d\n",pid);
        return ;
    }
}
void runProcessForeground(char* argv[]){
    pid_t pid=fork();
    if(pid==0)
    {
        if(execvp(argv[0],argv)<0){
            printf("Command not found\n");
        }
        exit(0);
    }
    else{
        wait(NULL);
        return ;
    }
}
void getPwd(char *path){
    getcwd(path, PATH_MAX);
}

void getUsername(char *username){
    getlogin_r(username, LOGIN_NAME_MAX); 
}

void getHostname(char *hostname){
    gethostname(hostname, HOST_NAME_MAX);
}

void pwdHomeParser(char *path){
    char *homepath = getenv("HOME");
    int minsize = strlen(homepath);
    if(minsize > strlen(path)){
        printf("%s", path);
    }
    else{
        char compstring[PATH_MAX+1];
        strncpy(compstring, path, minsize);
        compstring[minsize] = '\0';
        if(strcmp(compstring, homepath) == 0){
            printf("~%s", path + minsize);
        }
        else{
            printf("%s", homepath);
        }
    }
}

void promptDisplayer(){
    char path[PATH_MAX+1], username[LOGIN_NAME_MAX+1], hostname[HOST_NAME_MAX+1];
    getPwd(path);
    getUsername(username);
    getHostname(hostname);
    printf("<%s@%s:", username, hostname);
    pwdHomeParser(path);
    printf("> ");
}

void filePermissionDisplay(struct stat stt, char* tmp){
    int i = 0;
    memset(tmp, '-', 10);
    tmp[10] = '\0';
    if(++i && S_ISDIR(stt.st_mode)) tmp[i-1] = 'd';
    if(++i && stt.st_mode & S_IRUSR) tmp[i-1] = 'r';
    if(++i && stt.st_mode & S_IWUSR) tmp[i-1] = 'w';
    if(++i && stt.st_mode & S_IXUSR) tmp[i-1] = 'x';
    if(++i && stt.st_mode & S_IRGRP) tmp[i-1] = 'r';
    if(++i && stt.st_mode & S_IWGRP) tmp[i-1] = 'w';
    if(++i && stt.st_mode & S_IXGRP) tmp[i-1] = 'x';
    if(++i && stt.st_mode & S_IROTH) tmp[i-1] = 'r';
    if(++i && stt.st_mode & S_IWOTH) tmp[i-1] = 'w';
    if(++i && stt.st_mode & S_IXOTH) tmp[i-1] = 'x';
}

void echo(char *toprint){
    char *element, *str = toprint;
    element = strtok_r(str, "\"\'", &str);
    while(element){
        printf("%s", element);
        element = strtok_r(str, "\"\'", &str);
    }
    printf("\n");
}

void pwd(){
    char path[PATH_MAX+1];
    getPwd(path);
    printf("%s\n", path);
}

void cd(char *ndir){
    int ret;
    char temp[PATH_MAX + 1];
    getPwd(temp);
    if(ndir == NULL || strcmp(ndir, "~") == 0){
        ret = chdir(getenv("HOME"));
    }
    else if(strcmp(ndir, "-") == 0){
        ret = chdir(prevdir);
    }
    else{
        ret = chdir(ndir);
    }
    if(ret == -1){
        printf("Error\n");
    }
    else{
        strcpy(prevdir, temp);
    }
}

void ls(char* opt){
    char perm[11];
    char *timestring;
    int a = 0, l = 0;
    struct passwd *pwd;
    struct group *grp;
    DIR *dir;
    struct dirent *fil;
    struct stat stt;
    if(opt && ((strlen(opt) == 2 && opt[1] == 'a') ||
            (strlen(opt) == 3 && (opt[1] == 'a' || opt[2] == 'a'))))
        a = 1;
    if(opt && ((strlen(opt) == 2 && opt[1] == 'l') ||
            (strlen(opt) == 3 && (opt[1] == 'l' || opt[2] == 'l'))))
        l = 1;
    dir = opendir(".");
    while((fil = readdir(dir)) != NULL){
        if(a == 0 && fil->d_name[0] == '.') continue;
        if(l){
            lstat(fil->d_name, &stt);
            pwd = getpwuid(stt.st_uid);
            grp = getgrgid(stt.st_gid);
            timestring = ctime(&stt.st_mtime);
            timestring[strlen(timestring)-1] = '\0';
            filePermissionDisplay(stt, perm);
            printf("%s %d\t %s\t %s\t %jd\t", perm, (int)stt.st_nlink, pwd->pw_name, grp->gr_name, (intmax_t)stt.st_size);
            printf("%s\t", timestring+4);
            printf("%s\n", fil->d_name);
        }
        else printf("%s\n", fil->d_name);
    }
}

void runCommand(char* command){
    char *element, *recomm = command, *temp;
    element = strtok_r(recomm, " ", &recomm);
    if(strcmp(element, "echo") == 0){
        element = strtok_r(recomm, "\0", &recomm);
        echo(element);
    }
    else if(strcmp(element, "pwd") == 0){
        pwd();
    }
    else if(strcmp(element, "cd") == 0){
        element = strtok_r(recomm, " ", &recomm);
        cd(element);
    }
    else if(strcmp(element, "ls") == 0){
        element = strtok_r(recomm, " ", &recomm);
        temp = strtok_r(recomm, " ", &recomm);
        if(element && (temp || !(!strcmp("-la", element) || !strcmp("-a", element)
                    || !strcmp("-l", element) || !strcmp("-al", element)))){
            char* argv[1000] = {"ls",element,temp};
            int tm = 2;
            while(argv[tm]){
                argv[tm+1] = strtok_r(recomm, " ", &recomm);
                tm++;
            }
            runProcessForeground(argv);
        }
        else
            ls(element);
    }
    else if(strcmp(element, "quit") == 0){
        exit(0);
    }
    else if(strcmp(element, "clear") == 0){
        printf("\e[1;1H\e[2J");
    }
    else if(strcmp(element,"pinfo")==0){
            char *argv[]={"pinfo",0};
            pinfo(argv);
     }
    else{
        char* argv[1000]={element,0};
        int tp = 0;
        while(argv[tp]){
            argv[tp + 1] = strtok_r(recomm, " ", &recomm);
            tp++;
        }
        if(argv[tp-1] && !strcmp(argv[tp-1],"&")) runProcessBackground(argv);
        else runProcessForeground(argv);
    }
}

void separateCommand(char* prompt){
    char *single, *reprompt = prompt;
    single = strtok_r(reprompt, ";", &reprompt);
    while(single){
        runCommand(single);
        single = strtok_r(reprompt, ";", &reprompt);
    }
}

int main(){
    char prompt[1000];
    while(1){
        promptDisplayer();
        scanf("%[^\n]s", prompt);
        getchar();
        separateCommand(prompt);
        memset(prompt,0,sizeof prompt);
    }
}


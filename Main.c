#include<stdio.h>
#include<time.h>
#include<pwd.h>
#include<grp.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<limits.h>
#include<dirent.h>
#include<sys/stat.h>

// MacOS definitions 
#define MAC_OS
#ifdef MAC_OS
#define HOST_NAME_MAX 64
#define LOGIN_NAME_MAX 256
#endif
// End of MacOS definitions
//
// Global variables
char prevdir[PATH_MAX+1];
// End of global variables

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
            printf("%s %d\t %s\t %s\t %jd\t", perm, stt.st_nlink, pwd->pw_name, grp->gr_name, (intmax_t)stt.st_size);
            printf("%s\t", timestring+4);
            printf("%s\n", fil->d_name);
        }
        else printf("%s\n", fil->d_name);
    }
}

void runCommand(char* command){
    char *element, *recomm = command;
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
        ls(element);
    }
    else if(strcmp(element, "quit") == 0){
        exit(0);
    }
    else if(strcmp(element, "clear") == 0){
        printf("\e[1;1H\e[2J");
    }
    else{
        printf("ERROR: Command does not exist.\n");
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
    }
}


#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sched.h>
// MacOS definitions 
#define MAC_OS
#ifdef MAC_OS
#define HOST_NAME_MAX 64
#define LOGIN_NAME_MAX 256
#endif
// End of MacOS definitions
//
// Global variables
typedef struct job{
    char name[100];
    pid_t pid;
} job;

job     JOBS[1000];
int     sp;
char    prevdir[PATH_MAX+1];
// End of global variables



void execute_setenv(char *argv[]){
    char* var = argv[1];
    char* val = argv[2];
    setenv(val,var,1);
    printf("%s\n",getenv(val));
}

void fg(char *argv[]){
    int     job = atoi(argv[1]);
    int     i;
    int     act=0;

    for(i=0;i<sp;i++){
        if(kill(JOBS[i].pid,0)==0){
            act++;
            if(act==job){
                kill(JOBS[i].pid,SIGCONT);
                wait(NULL);
            }
           
        }
        else{
            fprintf(stderr, "%s", strerror(errno));
        }
    }
}

void bg(char *argv[]){
    int     job=atoi(argv[1]);
    int     i;
    int     act=0;

    for(i=0;i<sp;i++){
        if(kill(JOBS[i].pid,0)==0){
            act++;
            if(act==job){
                kill(JOBS[i].pid,SIGCONT);
            }
        }
        else{
            fprintf(stderr, "%s", strerror(errno));
        }
    }

}

void execute_unsetenv(char *argv[]){
    char*   var = argv[1];
    unsetenv(var);
}

void sig_handler(int sig){
    if(sig==SIGINT||sig==SIGSTOP)
        ;
    return ;
}

void overkill(){
    int     i;
    for(i=0;i<sp;i++){
        kill(JOBS[i].pid,9);
    }
}

void jobs(){
    int     i;
    int     act=0;

    for(i=0;i<sp;i++){
        int alive = kill(JOBS[i].pid,0);
        if(alive==0)
        {
            act++;
            printf("[%d] %d  %s\n",act,JOBS[i].pid,JOBS[i].name);
        }
        else{
            fprintf(stderr, "%s", strerror(errno));
        }
    }

}

void kjob(char *argv[]){
    int     job=atoi(argv[1]);
    int     sig=atoi(argv[2]);
    int     i;
    int     act=0;

    for(i=0;i<sp;i++){
        int alive = kill(JOBS[i].pid,0);
        if(alive==0){
            act++;
            if(act==job){
                kill(JOBS[i].pid,sig);
            }
        }
        else{
            fprintf(stderr, "%s\n", strerror(errno));
        }
    }

}

void proc_exit()
{
    union wait wstat;
    pid_t   pid;

    while (1) {
        pid = wait3(&wstat, WNOHANG, (struct rusage *)NULL );
        if (pid == 0){
            return;
        }
        else if (pid == -1)
            continue;
        else
            printf ("process with pid %d exited with Return code: %d\n",pid, wstat.w_retcode);
    }
}

void pinfo(char* argv[]){
    if(argv[1] == NULL){

        pid_t   pid= getpid();
        char    spid[50];
        char    status[10] = "/status";
        char    prox[100] = "/proc/";

        sprintf(spid,"%d",pid);
        strcat(spid,status);
        strcat(prox,spid);

        char *final[] = {"cat", prox, 0};

        pid_t pid_fork=fork();
        if(pid_fork==0){
            execvp(final[0],final);
            exit(0);
        }
        else if(pid_fork == -1){
            fprintf(stderr, "%s\n", strerror(errno));
        }
        else{
            wait(NULL);
        }

        char    proc[200] = "/proc/";
        char    nspid[100];
        char    exe[100]="/exe";

        sprintf(nspid,"%d",pid);
        strcat(proc,nspid);
        strcat(proc,exe);
        pid_t nfork = fork();

        if(nfork==0){
            char* get_exe[]={"readlink",proc,0};
            execvp(get_exe[0],get_exe);
            exit(0);
        }
        else if(nfork == -1){
            fprintf(stderr, "%s\n", strerror(errno));
        }
        else{
            wait(NULL);
        }
    }
    else{
        char    proc[100] = "/proc/";
        char    exe[200];
        char    status[50] = "/status";

        strcat(proc,argv[1]);
        strcpy(exe,proc);
        strcat(exe,"/exe");
        strcat(proc,status);

        char    *final[] = {"cat",proc,0};
        pid_t   pid_fork = fork();

        if(pid_fork == 0){
            if(execvp(final[0],final) == -1)
                printf("ERROR: No such pid value.");
            return;
        }
        else if(pid_fork == -1){
            fprintf(stderr, "%s",strerror(errno));
            return;
        }
        else{
            wait(NULL);
        }

        pid_t   nfork = fork();

        if(nfork==0)
        {
            char* get_exe[]={"readlink",exe,0};
            execvp(get_exe[0],get_exe);
            exit(0);
        }
        else{
            wait(NULL);
        }
    }

}

void runProcessBackground(char* argv[]){
    if(signal(SIGCHLD,proc_exit) == SIG_ERR){
        printf("Can't catch SIGCHILD\n");
        return;
    }

    pid_t pid   = fork();

    if(pid==0)
    {
        if(execvp(argv[0],argv)<0){
            fprintf(stderr, "%s\n", strerror(errno));
            printf("Command not found\n");
        }
        exit(0);
    }
    else if(pid == -1){
        fprintf(stderr, "%s\n", strerror(errno));
        return;
    }
    else{
        printf("pid:%d\n",pid);
        strcpy(JOBS[sp].name, argv[0]);
        JOBS[sp].pid = pid;
        sp++;
        printf("%s\n", JOBS[sp-1].name);
        return;
    }

}

void runProcessForeground(char* argv[]){
    pid_t   pid = fork();
    if(pid==0)
    {
        if(execvp(argv[0],argv)<0){
            fprintf(stderr, "%s\n", strerror(errno));
            printf("Command not found\n");
        }
        exit(0);
    }
    else if(pid == -1){
        fprintf(stderr, "%s\n", strerror(errno));
        return;
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
    char    *homepath = getenv("HOME");
    int     minsize = strlen(homepath);

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
    char    path[PATH_MAX+1];
    char    username[LOGIN_NAME_MAX+1];
    char    hostname[HOST_NAME_MAX+1];
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

void echo(char *toprint[]){
    int     t = 1;

    while(toprint[t]){
        if(t > 1) printf(" ");

        char    *element;
        char    *str = toprint[t];

        element = strtok_r(str, "\"\'", &str);
        while(element){
            printf("%s", element);
            element = strtok_r(str, "\"\'", &str);
        }
        t++;
    }
    printf("\n");
}

void pwd(){
    char    path[PATH_MAX+1];

    getPwd(path);
    printf("%s\n", path);
}

void cd(char *ndir){
    int     ret;
    char    temp[PATH_MAX + 1];

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
        fprintf(stderr,"%s\n",strerror(errno));
    }
    else{
        strcpy(prevdir, temp);
    }
}

void ls(char* opt){
    char    perm[11];
    char    *timestring;
    int     a = 0;
    int     l = 0;
    DIR     *dir;
    struct passwd   *pwd;
    struct group    *grp;
    struct dirent   *fil;
    struct stat     stt;

    if(opt && ((strlen(opt) == 2 && opt[1] == 'a') ||
                (strlen(opt) == 3 && (opt[1] == 'a' || opt[2] == 'a'))))
        a = 1;

    if(opt && ((strlen(opt) == 2 && opt[1] == 'l') ||
                (strlen(opt) == 3 && (opt[1] == 'l' || opt[2] == 'l'))))
        l = 1;

    dir = opendir(".");
    if(dir == NULL){
        fprintf(stderr,"%s\n",strerror(errno));
        return;
    }

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
    char* argv[1000]={element,0};
    int tp = 0;
    while(argv[tp]){
        argv[tp + 1] = strtok_r(recomm, " ", &recomm);
        tp++;
    }

    int inp = 0, out = 0, outw = 0;
    char inpf[256] = {0}, outf[256] = {0};
    int sm;
    int INP, OUT;
    int save_in, save_out;
    if(tp > 2 && argv[tp-1][strlen(argv[tp-1])-1] != '\"')
    {
        if(!strcmp(argv[tp-2], "<")){
            inp = 1;
            strcpy(inpf, argv[tp-1]);
            if(tp >= 5 && argv[tp-3][strlen(argv[tp-3])-1] != '\"'){
                if(!strcmp(argv[tp-4],">")){
                    out = 1;
                    strcpy(outf, argv[tp-3]);
                }
                else if(!strcmp(argv[tp-4],">>")){
                    outw = 1;
                    strcpy(outf, argv[tp-3]);
                }
            }
        }
        else if(!strcmp(argv[tp-2], ">>")){
            outw = 1;
            strcpy(outf, argv[tp-1]);
            if(tp >= 5 && argv[tp-3][strlen(argv[tp-3])-1] != '\"'){
                if(!strcmp(argv[tp-4],"<")){
                    inp = 1;
                    strcpy(inpf, argv[tp-3]);
                }
            }
        }
        else if(!strcmp(argv[tp-2], ">")){
            out = 1;
            strcpy(outf, argv[tp-1]);
            if(tp >= 5 && argv[tp-3][strlen(argv[tp-3])-1] != '\"'){
                if(!strcmp(argv[tp-4],"<")){
                    inp = 1;
                    strcpy(inpf, argv[tp-3]);
                }
            }
        }
        sm = inp + out + outw;
        if(sm >= 1){
            argv[tp - 1] = argv[tp - 2] = 0;
            tp-=2;
            if(sm == 2){
                argv[tp - 1] = argv[tp - 2] = 0;
                tp-=2;
            }
        }
        if(out == 1 || outw == 1){
            if(out == 1)
                OUT = open(outf, O_CREAT|O_WRONLY|O_TRUNC|S_IRWXU);
            else if(outw == 1)
                OUT = open(outf, O_WRONLY|O_CREAT|O_APPEND);
            save_out = dup(fileno(stdout));
            dup2(OUT, fileno(stdout));
            fflush(stdout);
            close(OUT);
        }
        if(inp == 1){
            INP = open(inpf, O_RDONLY);
            save_in = dup(fileno(stdin));
            dup2(INP, fileno(stdin));
            close(INP);
        }
    }


    if(strcmp(element, "echo") == 0){
        echo(argv);
    }
    else if(strcmp(element, "pwd") == 0){
        pwd();
    }
    else if(strcmp(element, "cd") == 0){
        cd(argv[1]);
    }
    else if(strcmp(element, "ls") == 0){
        element = argv[1];
        temp = argv[2];
        if(element && (temp || !(!strcmp("-la", element) || !strcmp("-a", element)
                        || !strcmp("-l", element) || !strcmp("-al", element)))){
            runProcessForeground(argv);
        }
        else
            ls(element);
    }
    else if(strcmp(element,"jobs")==0){
        jobs();
    }
    else if(strcmp(element,"fg")==0){
        fg(argv);
    }
    else if(strcmp(element, "quit") == 0){
        exit(0);
    }
    else if(strcmp(element,"bg")==0){
        bg(argv);
    }
    else if(strcmp(element,"setenv")==0){
        execute_setenv(argv);
    }
    else if(strcmp(element,"unsetenv")==0){
        execute_unsetenv(argv);
    }
    else if(strcmp(element,"overkill")==0){
        overkill();
    }
    else if(strcmp(element,"kjob")==0){
        kjob(argv);
    }
    else if(strcmp(element, "clear") == 0){
        printf("\e[1;1H\e[2J");
    }
    else if(strcmp(element,"pinfo")==0){
        char *argv2[]={"pinfo",0};
        pinfo(argv2);
    }
    else{
        if(sm > 0) runProcessForeground(argv);
        else{
            if(argv[tp-1] && !strcmp(argv[tp-1],"&")) runProcessBackground(argv);
            else runProcessForeground(argv);
        }
    }

    if(out == 1 || outw == 1){
        OUT = 1;
        dup2(save_out, fileno(stdout));
        close(save_out);
    }
    if(inp == 1){
        INP = 0;
        dup2(save_in, fileno(stdin));
        close(save_in);
    }
}

void runPipe(char* command){

    int     p[2];
    pid_t   pid;
    int     fd_in = 0;
    char*   single = strtok_r(command, "|", &command);
    char*   doub = strtok_r(command, "|", &command);

    if(doub == NULL) {runCommand(single); return;}

    while(single){
        if(pipe(p) == -1){
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if((pid = fork()) == -1){
            exit(EXIT_FAILURE);
        }
        else if(pid == 0){

            if(dup2(fd_in, 0) == -1){
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            if(doub != NULL){
                if(dup2(p[1], 1) == -1){
                    fprintf(stderr, "%s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }

            if(close(p[0]) == -1){
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            runCommand(single);
            exit(EXIT_FAILURE);
        }
        else{
            wait(NULL);
            if(close(p[1]) == -1){
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            fd_in = p[0];
        }

        single = doub;
        doub = strtok_r(command, "|", &command);
    }
}

void separateCommand(char* prompt){
    char    *reprompt = prompt;
    char    *single = strtok_r(reprompt, ";", &reprompt);
    while(single){
        runPipe(single);
        single = strtok_r(reprompt, ";", &reprompt);
    }
}

void signalInit(){
    if(signal(SIGINT,SIG_IGN) == SIG_ERR)
        printf("Can't catch SIGINT\n");




    if(signal(SIGINT,sig_handler) == SIG_ERR)
        printf("Can't catch SIGINT\n");



    if(signal(SIGTSTP,sig_handler) == SIG_ERR)
        printf("Can't catch SIGTSTP\n");
}

int main(){
    char    prompt[1000];
    while(1){
        signalInit();
        promptDisplayer();
        scanf("%[^\n]s", prompt);
        getchar();
        separateCommand(prompt);
        memset(prompt,0,sizeof prompt);
    }
}


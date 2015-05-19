#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>


#define WRITE 1
#define READ 0
#define TRUE 1
#define FALSE 0

#define MAX_LENGTH 80
#define DELIMS " \t\n"

char *cmd;
bool poll;

void closePipe(int pipeEnd[2], int direction){
    if (close(pipeEnd[direction]) == -1)
        perror("error when closing pipe");
    exit(1);
}

void type_prompt(){
    if(poll){
        if(waitpid(-1,NULL, WNOHANG)>0){
            printf("Background child process termineted\n");
        }
    }
    printf("fake_shell$ ");
}

void register_signalhandler(int signal_code, void (*handler) (int sig)){
    int return_value;
    struct sigaction signal_parameters;

    signal_parameters.sa_handler = handler;
    sigemptyset(&signal_parameters.sa_mask);
    signal_parameters.sa_flags = SA_RESTART;

    return_value = sigaction(signal_code, &signal_parameters, (void *) 0);

    if(-1 == return_value) {
        perror("sigaction() faild\n");
        exit(1);
    }
}

void signal_handler(int signal_code){
    if( SIGCHLD == signal_code ) {
        int pidChild, status;
        if((pidChild = waitpid(-1, &status, WNOHANG)) != 0)
        {
            if(-1 == pidChild)
            {
                perror("Waitpid failed");
            }
            else
            {
                printf("Background process %d has terminated\n", pidChild);
            }
        }
    }
    return;
}

int handleCommand(bool bg, char *param[7]){
    pid_t pid;
    int status = -1;

    struct timeval start, end;
    double timeUsed;
    pid = fork();
    if (pid == -1) {
        perror("\nfork\n");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0) {/*in child*/
        register_signalhandler(SIGCHLD, signal_handler);
        if(execvp(param[0], param) < 0){
            perror("Could not execute command");
        }
        exit(1);
    }
    else{
        if(bg){
            if (!poll) {
                register_signalhandler(SIGCHLD, signal_handler);
            }
        }
        else {
            sighold(SIGCHLD);
            gettimeofday(&start, NULL);
            waitpid(pid, &status, 0);
            gettimeofday(&end, NULL);
            timeUsed = (end.tv_sec + ((double)end.tv_usec/1000000))-(start.tv_sec+((double)start.tv_usec/1000000));
            printf("Process terminated in: %f seconds\n", timeUsed);
            sigrelse(SIGCHLD);
        }
    }
    return 0;
}

bool checkForBackgroundP(char *arg){
    if(strcmp(arg, "&") == 0) {
        return true;
    }
    return false;
}

int handleCd(char *arg)
{
    char *dir;
    if (!arg) chdir(getenv("HOME"));
    else chdir(arg);
    dir = getcwd(NULL, 0);
    setenv("PWD", dir, 1);
    return 0;
}

int handleCheckEnv(char *arg) {
    pid_t childPID, childPID2, childPID3;
    int pipe1[2], pipe2[2];

    if (pipe(pipe1) == -1) {
        perror("pipe1");
        return EXIT_FAILURE;
    }
    if (pipe(pipe2) == -1) {
        perror("pipe2");
        return EXIT_FAILURE;
    }
    sighold(SIGCHLD);
    if ((childPID = fork()) >= 0) { /*child*/
        if (childPID == 0) {
            close(pipe1[READ]);
            if (dup2(pipe1[WRITE], STDOUT_FILENO) == -1) {
                perror("dup2 failed");
                return EXIT_FAILURE;
            }
            close(pipe1[WRITE]);
            if (execlp("printenv", "printenv", arg, NULL) == -1) {
                perror("printenv failed");
                return EXIT_FAILURE;
            }
        }
        else {
            waitpid(childPID, NULL, 0);
            close(pipe1[WRITE]);
            childPID2 = fork();
            if (childPID2 >= 0) {
                if (childPID2 == 0) {
                    if (dup2(pipe1[READ], STDIN_FILENO) == -1) {
                        perror("dup2 failed");
                        return EXIT_FAILURE;
                    }
                    close(pipe1[READ]);
                    close(pipe2[READ]);
                    if (dup2(pipe2[WRITE], STDOUT_FILENO) == -1) {
                        perror("dup2 failed");
                        return EXIT_FAILURE;
                    }
                    close(pipe2[WRITE]);
                    if (execlp("sort", "sort", NULL) == -1) {
                        perror("Sort failed");
                        return EXIT_FAILURE;
                    }
                }
                else {
                    waitpid(childPID2, NULL, 0);
                    close(pipe1[READ]);
                    close(pipe2[WRITE]);
                    childPID3 = fork();
                    if (childPID3 >= 0) {
                        if (childPID3 == 0) {
                            char *pager = getenv("PAGER");
                            if (dup2(pipe2[READ], STDIN_FILENO) == -1) {
                                perror("dup2 failed");
                            }
                            close(pipe2[READ]);

                            if (!pager) {
                                pager = "less";
                            }
                            if (execlp(pager, pager, NULL) == -1) {
                                perror("Pager failed, do MORE");
                                execlp("more", "more", NULL);
                            }
                        } else {
                            waitpid(childPID3, NULL, 0);
                            close(pipe2[READ]);
                        }
                    }
                }
            }
            else{
                perror("fork failed!");
                return EXIT_FAILURE;
            }
        }
    }
    else {
        perror("fork failed!");
        return EXIT_FAILURE;
    }
    sigrelse(SIGCHLD);
    return 0;
}

int handleLs(){
    execlp("ls", "ls", "-a", NULL);
    return 0;
}

int main(int argc, char *argv[]) {
    pid_t pid;
    char *line;
    char *param[7];
    poll = false;

    #ifdef SIGNAL /* test whether SIGNAL is defined...*/
            if(SIGNAL == 1) printf("\nSIGNAL Defined\n");
            else {
                poll = true;
                printf("\nPOLL Defined\n");
            }
        #else /*otherwise use POLL...*/
        poll = true;
        printf("\nPOLL Defined\n");
    #endif

    if(signal(SIGINT, SIG_IGN) != SIG_IGN) signal(SIGINT, SIG_IGN);

    while (TRUE) {
        type_prompt();
        fflush(stdout);

        line = (char *)malloc(MAX_LENGTH+1);
        if (!fgets(line, MAX_LENGTH, stdin)) break;
        if ((cmd = strtok(line, DELIMS))) {
            errno = 0;
            if (strcmp(cmd, "cd") == 0) {
                char *arg = strtok(0, DELIMS);
                handleCd(arg);
            }
            else if (strcmp(cmd, "ls") == 0) {
                if ((pid = fork()) == -1) {
                    perror("ls Fork");
                    return EXIT_FAILURE;
                }
                if (pid == 0) {
                    handleLs();
                }else{
                    sighold(SIGCHLD);
                    waitpid(pid, NULL, 0);
                    sigrelse(SIGCHLD);
                }
            }

            else if (strcmp(cmd, "exit") == 0) {
                /*here we should terminate all remaining processes
                 started from the shell in an orderly manner before exiting the shell itself*/
                break;
            } else if (strcmp(cmd, "checkEnv") == 0) {
                char *arg = strtok(0, DELIMS);
                handleCheckEnv(arg);
            } else {
                char *tmp = strtok(0, DELIMS);
                int counter = 0;
                bool bg;
                param[0] = cmd;
                while((tmp!=NULL) && (counter <6)){
                    counter++;
                    param[counter] = tmp;
                    tmp = strtok(0, DELIMS);
                }
                bg = checkForBackgroundP(param[counter]);
                if(bg) param[counter] = NULL;
                else param[counter +1] = NULL;
                handleCommand(bg, param);
            }
            if (errno) perror("Command failed");
        }
        free(line);
    }
}

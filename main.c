#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>


#define WRITE 1
#define READ 0
#define TRUE 1
#define FALSE 0

#define MAX_LENGTH 80
#define DELIMS " \t\n"

char *cmd;

void closePipe(int pipeEnd[2], int direction){
    if (close(pipeEnd[direction]) == -1)
        perror("error when closing pipe");
    exit(1);
}
void type_prompt(){
    printf("fake_shell$ ");
}

int handleCommand(bool bg, char *param[7]){
    pid_t pid;
    struct timeval start, end;
    double timeUsed;
    pid = fork();
    if(pid >= 0){
        printf("fork\n");
        if(pid == 0) {
            printf("in child handleC\n");
            if(execvp(param[0], param) < 0){
                perror("Could not execute command");
            }
            return 0;
        }
        else{
            if(!bg){
                gettimeofday(&start, NULL);
                waitpid(pid, NULL, 0);
                gettimeofday(&end, NULL);
                timeUsed = (end.tv_sec + ((double)end.tv_usec/1000000))-(start.tv_sec+((double)start.tv_usec/1000000));
                printf("Process terminated in: %f seconds\n", timeUsed);
            }else{
                //TODO signals

            }
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
    printf("called function\n");

    if (pipe(pipe1) == -1) {
        perror("pipe1");
        return EXIT_FAILURE;
    }
    if (pipe(pipe2) == -1) {
        perror("pipe2");
        return EXIT_FAILURE;
    }
    printf("pipes created\n");
    if (errno) perror("Command failed first");

    if ((childPID = fork()) >= 0) { //child
        printf("forkaaaat\n");
        if (childPID == 0) {
            printf("in child\n");
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
            printf("end of child\n");
        }
        else {
            waitpid(childPID, NULL, 0);
            printf("in parent\n");
            close(pipe1[WRITE]);
            childPID2 = fork();
            if (childPID2 >= 0) {
                printf("fork2\n");
                if (childPID2 == 0) {
                    printf("child 2\n");
                    close(pipe1[WRITE]);
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
                    printf("parent 2\n");
                    close(pipe1[READ]);
                    close(pipe1[WRITE]);
                    close(pipe2[WRITE]);
                    childPID3 = fork();
                    if (childPID3 >= 0) {
                        printf("fork 3\n");
                        if (errno) perror("Command failed both");
                        if (childPID3 == 0) {
                            char *pager = getenv("PAGER");
                            if (errno) perror("Command failed");
                            printf("child 3\n");
                            close(pipe2[WRITE]);
                            if (dup2(pipe2[READ], STDIN_FILENO) == -1) {
                                perror("dup2 failed");
                            }
                            close(pipe2[READ]);

                            if (!pager) {
                                pager = "less";
                            }
                            if (execlp(pager, pager, NULL) == -1) {
                                perror("Pager failed, do more");
                                execlp("more", "more", NULL);
                            }
                        } else {
                            waitpid(childPID3, NULL, 0);
                            printf("parent 3\n");
                            close(pipe2[READ]);
                            close(pipe2[WRITE]);
                            if (errno) perror("Command failed prent");
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
    return 0;
}

int handleLs(){

    execlp("ls", "ls", "-a", NULL);
    return 0;
}

//TODO skriv ut vilka kommandon det är som inte funkade i en pipe
int main(int argc, char *argv[]) {
    pid_t pid;
    char *line;
    char *input;
    char *param[7];
    while (TRUE) {
        type_prompt();
        fflush(stdout);

        line = (char *)malloc(MAX_LENGTH+1);
        if (!fgets(line, MAX_LENGTH, stdin)) break;
        input = line;
        if ((cmd = strtok(line, DELIMS))) {
            // Clear errors
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
                    waitpid(pid, NULL, 0);

                }
            }

            else if (strcmp(cmd, "exit") == 0) {
                //here we should terminate all remaining processes
                // started from the shell in an orderly manner before exiting the shell itself
                break;
            } else if (strcmp(cmd, "checkEnv") == 0) {
                char *arg = strtok(0, DELIMS);
                printf("found checkEnv\n");
                handleCheckEnv(arg);
                printf("DONE!\n");
                errno = 0;
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
                param[6]=NULL;
                bg = checkForBackgroundP(param[counter]);//TODO global?
                handleCommand(bg, param);
            }
            if (errno) perror("Command failed");
        }
    }
}

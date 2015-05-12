#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>


#define WRITE 1
#define READ 0

#define MAX_LENGTH 80
#define DELIMS " \t\n"

int pipe1[2], pipe2[2];
void closePipe(int pipeEnd[2]){
    if (close(pipeEnd[READ]) == -1)
        err("error when closing pipe, read");

    if (close(pipeEnd[WRITE]) == -1)
        err("error when closing pipe, write");
}
int main(int argc, char *argv[]) {
    char *cmd;
    char line[MAX_LENGTH];
    pid_t childPID, childPID2, childPID3;





    while (1) {
        printf("fake_shell$ ");
        //The C library function char *fgets(char *str, int n, FILE *stream) reads
        // a line from the specified stream and stores it into the string pointed
        // to by str. It stops when either (n-1) characters are read, the newline
        // character is read, or the end-of-file is reached, whichever comes first
        if (!fgets(line, MAX_LENGTH, stdin)) break;

        // Parse and execute command
        //The C library function char *strtok(char *str, const char *deli
        // breaks string str into a series of tokens using the delimiter delim.
        if ((cmd = strtok(line, DELIMS))) {
            // Clear errors
            errno = 0;

            if (strcmp(cmd, "cd") == 0) {
                char *arg = strtok(0, DELIMS);
                int a = handleCd(arg);
            }
            else if (strcmp(cmd, "ls") == 0) {
                childPID = fork();
                if (childPID >= 0) {
                    if (childPID == 0) {
                        int a = handleLs();
                    }
                    else {
                        printf("dekfjsf");
                    }
                }
                else {
                    printf("fork failed!");

                }
                if (strcmp(cmd, "exit") == 0) {
                    //here we should terminate all remaining processes
                    // started from the shell in an orderly manner before exiting the shell itself
                    break;
                } else if (strcmp(cmd, "checkEnv") == 0) {
                    if (pipe(pipe1) == -1) {
                        err("pipe1");
                    }
                    if (pipe(pipe2) == -1) {
                        err("pipe2");
                    }
                    char *arg = strtok(0, DELIMS);
                    childPID = fork();
                    if (childPID >= 0) {
                        if (childPID == 0) {

                            if (dup2(pipe1[WRITE], STDOUT_FILENO) == -1) {
                                perror("dup2 failed");
                            }
                            closePipe(pipe1);
                            closePipe(pipe2);
                            if (execlp("printenv", "printenv", arg, NULL) == -1) {
                                perror("printenv faild");
                            }

                        }
                        else {
                            closePipe(pipe1);
                            closePipe(pipe2);
                            printf("gnu");
                            waitpid(childPID, NULL, 0);
                            childPID2 = fork();
                            if (childPID2 >= 0) {
                                if (childPID2 == 0) {
                                    if (dup2(pipe1[READ], STDIN_FILENO) == -1) {
                                        perror("dup2 failed");
                                    }
                                    if (dup2(pipe2[WRITE], STDOUT_FILENO) == -1) {
                                        perror("dup2 failed");
                                    }
                                    closePipe(pipe1);
                                    closePipe(pipe2);
                                    if (execlp("sort", "sort", NULL) == -1) {
                                        perror("Sort failed");
                                    }
                                }
                                else {
                                    closePipe(pipe1);
                                    closePipe(pipe2);
                                    childPID3 = fork();
                                    if (childPID3 >= 0) {
                                        if (childPID3 == 0) {
                                            if (dup2(pipe2[READ], STDIN_FILENO) == -1) {
                                                perror("dup2 failed");
                                            }
                                            closePipe(pipe1);
                                            closePipe(pipe2);
                                            char *pager = getenv("PAGER");

                                            if (!pager) {
                                                pager = "less";
                                            }
                                            if (execlp(pager, pager, NULL) == -1) {
                                                perror("Pager faild, do more");
                                                execlp("more", "more", NULL);
                                            }
                                        } else {
                                            closePipe(pipe1);
                                            closePipe(pipe2);
                                        }
                                    }
                                }
                            }
                            else{
                                perror("fork failed!");
                            }
                        }
                    }
                    else {
                        perror("fork failed!");
                        return 1;
                    }
                } else {
                    errno = ENOSYS;
                } //system(line);

                if (errno) perror("Command failed");
            }

        }

        return 0;
//    fprintf(stderr, "cd missing argument.\n")

    }
}
int handleCd(char *arg)
{
    if (!arg) chdir(getenv("HOME"));
    else chdir(arg);
    return 0;
}

int handleCheckEnv(char *arg) {

    char *grep = (" | grep ");
    char *sort = (" | sort | ");
    char *pager = getenv("PAGER");

    if (!pager) {
        pager="less";
    }
    char str[100];
    strcpy(str, "printenv");
    if (arg){
        strcat(str, grep);
        strcat(str, arg);
    }
    strcat(str, sort);
    strcat(str, pager);
    //system(str);
    execlp("printenv", "printenv", arg, NULL);
    /*if (errno && strcmp(pager, "more")) {
        char *pager = "more";
        char str[100];
        strcpy(str, "printenv");
        if (arg){
            strcat(str, grep);
            strcat(str, arg);
        }
        strcat(str, sort);
        strcat(str, pager);
        system(str);
    }*/


    return 0;
}

int handleLs(){

    execlp("ls", "ls", "-a", NULL);
    return 0;
}


//TODO skriv ut vilka kommandon det är som inte funkade i en pipe
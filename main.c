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
#define _XOPEN_SOURCE 500

char *cmd;
bool poll;

void type_prompt(){
    /*
     * If polling is chosen, check if any child processes have terminated.
     * If waitpid returns -1 and error has occured, if it returns 0
     * no child processes have changed status. Otherwise waitpid returns
     * the pid of the child.
     */
    if(poll){
        int pidChild;
        if((pidChild = waitpid(-1,NULL, WNOHANG)) != 0){
            if (pidChild == -1) {
                perror("Waitpid failed");
            }
            else printf("Background process %d has terminated\n", pidChild);
        }
    }
    /*
     * Print the regular prompt.
     */
    printf("fake_shell$ ");
}

void register_signalhandler(int signal_code, void (*handler) (int sig)){
    int return_value;
    struct sigaction signal_parameters;

    /*
     * Use the handler given by the functions argument. We will not
     * use any masks. The flag SA_RESTART has been set to avoid interrupt errors
     * when the signal handler returns.
     */
    signal_parameters.sa_handler = handler;
    sigemptyset(&signal_parameters.sa_mask);
    signal_parameters.sa_flags = SA_RESTART;

    return_value = sigaction(signal_code, &signal_parameters, (void *) 0);

    /*
     * If sigaction returns -1 an error has occured.
     */
    if(-1 == return_value) {
        perror("sigaction() faild\n");
        exit(1);
    }
}

void signal_handler(int signal_code){
    /*
     * When a SIGCHLD signal is received, waitpid will check
     * if any child processes have changed status and if so return their pid.
     * If waitpid returns -1 and error has occured, if it returns 0
     * no child processes have changed status.
     */
    if( SIGCHLD == signal_code ) {
        int pidChild;
        if((pidChild = waitpid(-1, NULL, WNOHANG)) != 0)
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

/*
 * Function to handle commands that are not built in
 * to the shell.
 */
int handleCommand(bool bg, char *param[7]){
    pid_t pid;
    struct timeval start, end; /*Saves start and end time of process.*/
    double timeUsed; /*Will hold the total time consumed.*/
    pid = fork();
    if (pid == -1) { /*Fork returns -1 if it fails.*/
        perror("\nfork\n");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0) { /*In the child.*/
        /*
         * If execvp returns -1 an error has occured. Otherwise it will
         * terminate the child when finished.
         */
        if(execvp(param[0], param) < 0){
            perror("Could not execute command");
        }
        exit(1);
    }
    else{ /*In the parent.*/
        if(bg){ /*If background has been chosen.*/
            if (!poll) { /*If SIGDET has been defined.*/
                /*
                 * Register a signalhandler to listen for signals
                 * from children that terminate.
                 */
                register_signalhandler(SIGCHLD, signal_handler);
            }
        }
        else { /*Foreground process has been chosen.*/
            sighold(SIGCHLD);  /*Don't let signals interrupt.*/
            gettimeofday(&start, NULL);
            waitpid(pid, NULL, 0); /*Wait for the child to terminate.*/
            gettimeofday(&end, NULL);
            /*
             * Calculating time used. Timeval stores time as seconds and microseconds.
             * They are combined and stored as a double instead.
             */
            timeUsed = (end.tv_sec + ((double)end.tv_usec/1000000))-(start.tv_sec+((double)start.tv_usec/1000000));
            printf("Process terminated in: %f seconds\n", timeUsed);
            sigrelse(SIGCHLD); /*Let signals through again.*/
        }
    }
    return 0;
}

/*
 * If the user wants a process to be run in the background,
 * & is added at the end of the command. This function returns
 * true if & was found at the end, otherwise false.
 */
bool checkForBackgroundP(char *arg){
    if(strcmp(arg, "&") == 0) {
        return true;
    }
    return false;
}

/*
 * This function handles the commands needed to change directory.
 * It uses the built in command chdir, and if no directory has been
 * specified, changes to the HOME directory. Also manually updates the
 * environment parent working directory to the new current working
 * directory.
 */
void handleCd(char *arg)
{
    char *dir;
    if (!arg) chdir(getenv("HOME"));
    else chdir(arg);
    dir = getcwd(NULL, 0);
    setenv("PWD", dir, 1);
    return;
}

/*
 * This function does what "printenv | grep <arguments> | sort | pager"
 * usually does in a shell. Pipes are used to transfer results between
 * children, instead of them printing to stdout and reading from stdin.
 * The parent always waits for the child to finish, since the results
 * are used by other children.
 */
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
    sighold(SIGCHLD); /*Don't let signals interrupt.*/
    if ((childPID = fork()) >= 0) { /*In first child.*/
        if (childPID == 0) {
            close(pipe1[READ]);
            /*
             * Reroute stdout to pipe1.
             */
            if (dup2(pipe1[WRITE], STDOUT_FILENO) == -1) {
                perror("dup2 failed");
                return EXIT_FAILURE;
            }
            close(pipe1[WRITE]);
            /*
             * Execute printenv with an argument if given by user.
             */
            if (execlp("printenv", "printenv", arg, NULL) == -1) {
                perror("printenv failed");
                return EXIT_FAILURE;
            }
        }
        else { /*In parent.*/
            waitpid(childPID, NULL, 0);
            close(pipe1[WRITE]);
            childPID2 = fork();
            if (childPID2 >= 0) {
                if (childPID2 == 0) { /*In second child.*/
                    /*
                     * Reroute stdin to pipe1.
                     */
                    if (dup2(pipe1[READ], STDIN_FILENO) == -1) {
                        perror("dup2 failed");
                        return EXIT_FAILURE;
                    }
                    close(pipe1[READ]);
                    close(pipe2[READ]);
                    /*
                     * Reroute stdout to pipe2.
                     */
                    if (dup2(pipe2[WRITE], STDOUT_FILENO) == -1) {
                        perror("dup2 failed");
                        return EXIT_FAILURE;
                    }
                    close(pipe2[WRITE]);
                    /*
                     * Execute sort on result from printenv.
                     */
                    if (execlp("sort", "sort", NULL) == -1) {
                        perror("Sort failed");
                        return EXIT_FAILURE;
                    }
                }
                else { /*In parent.*/
                    waitpid(childPID2, NULL, 0);
                    close(pipe1[READ]);
                    close(pipe2[WRITE]);
                    childPID3 = fork();
                    if (childPID3 >= 0) {
                        if (childPID3 == 0) { /*In third child.*/
                            char *pager = getenv("PAGER");
                            if (dup2(pipe2[READ], STDIN_FILENO) == -1) {
                                perror("dup2 failed");
                            }
                            close(pipe2[READ]);

                            if (!pager) {
                                pager = "less";
                            }
                            /*
                             * Print result using the pager specified in environment,
                             * unless it is less. If it fails, set pager to less.
                             */
                            if ((strcmp(pager, "less")>0) && (execlp(pager, pager, NULL) == -1)) {
                                perror("Pager failed, use less");
                                pager = "less";
                            }
                            /*
                             * Try printing using less, if that fails, try to use more.
                             */
                            if ((strcmp(pager, "less") == 0) && (execlp(pager, pager, NULL) == -1)) {
                                perror("Less failed, do more");
                                execlp("more", "more", NULL);
                            }
                        } else { /*In parent.*/
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
    sigrelse(SIGCHLD); /*Let signals through again.*/
    return 0;
}

/*
 * When user calls ls, automatically use ls -a.
 */
int handleLs(){
    execlp("ls", "ls", "-a", NULL);
    return 0;
}

int main(int argc, char *argv[]) {
    pid_t pid;
    char *line;
    char *param[7];
    poll = false;

    #ifdef SIGDET /* test whether SIGDET is defined...*/
            if(SIGDET == 1) printf("\nSIGDET Defined\n");
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
                printf("Shell will terminate when all children have finished\n");
                while (waitpid(-1, NULL, 0) > 0){
                }
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

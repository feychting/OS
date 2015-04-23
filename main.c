#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


#define MAX_LENGTH 80
#define DELIMS " \t\n"

int main(int argc, char *argv[]) {
    char *cmd;
    char line[MAX_LENGTH];

    while (1) {
        printf("fake_shell$ ");
        //The C library function char *fgets(char *str, int n, FILE *stream) reads
        // a line from the specified stream and stores it into the string pointed
        // to by str. It stops when either (n-1) characters are read, the newline
        // character is read, or the end-of-file is reached, whichever comes firs
        if (!fgets(line, MAX_LENGTH, stdin)) break;

        // Parse and execute command
        //The C library function char *strtok(char *str, const char *delim)
        // breaks string str into a series of tokens using the delimiter delim.
        if ((cmd = strtok(line, DELIMS))) {
            // Clear errors
            errno = 0;

            if (strcmp(cmd, "cd") == 0) {
                char *arg = strtok(0, DELIMS);

                if (!arg) fprintf(stderr, "cd missing argument.\n");
                else chdir(arg);

            } else if (strcmp(cmd, "exit") == 0) {
                //here we should terminates all remaining processes
                // started from the shell in an orderly manner before exiting the shell itself
                break;

            } else system(line);

            if (errno) perror("Command failed");
        }
    }

    return 0;
}

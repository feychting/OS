#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#define chdir _chdir

#else
#include <unistd.h>
#endif

#define MAX_LENGTH 80
#define DELIMS " \t\r\n"

int main(int argc, char *argv[]) {
    char *cmd;
    char line[MAX_LENGTH];

    while (1) {
        printf("fake_shell$ ");
        if (!fgets(line, MAX_LENGTH, stdin)) break;

        // Parse and execute command
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

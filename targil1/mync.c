#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s -e \"<program> <args>\"\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-e") != 0) {
        usage(argv[0]);
    }

    char *cmd = argv[2];

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Child process
        // Redirect stdin, stdout, and stderr
        if (dup2(STDIN_FILENO, STDIN_FILENO) == -1 ||
            dup2(STDOUT_FILENO, STDOUT_FILENO) == -1 ||
            dup2(STDERR_FILENO, STDERR_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        char *args[] = {"/bin/sh", "-c", cmd, NULL};
        execvp(args[0], args); //is used to execute the command passed as an argument

        // If execvp returns, there was an error, if not its exit from the procces
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {  // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child killed by signal %d\n", WTERMSIG(status));
        }
    }

    return 0;
}

//the child procces is used to execute the program
//the perent procces print if the procces is succeed or not 
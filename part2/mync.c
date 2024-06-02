#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char *exec_program = NULL;
    int opt;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "e:")) != -1) {
        switch (opt) {
            case 'e':
                exec_program = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -e \"program args\"\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (exec_program == NULL) {
        fprintf(stderr, "Usage: %s -e \"program args\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Split exec_program into command and arguments
    char *args[256];
    int i = 0;
    char *token = strtok(exec_program, " ");
    while (token != NULL && i < 255) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Null-terminate the arguments array

    // Execute the program with redirected input/output
    if (execvp(args[0], args) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    return 0;
}

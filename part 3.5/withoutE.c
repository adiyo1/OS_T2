#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>

#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int start_tcp_server(int port) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int opt = 1;

    printf("Starting TCP server on port %d\n", port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        error("ERROR on setsockopt");

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    printf("Client connected on port %d\n", port);
    close(sockfd);
    return newsockfd;
}

int start_tcp_client(const char *hostname, int port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    printf("Connecting to TCP server %s on port %d\n", hostname, port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *) &serv_addr.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    printf("Connected to TCP server %s on port %d\n", hostname, port);
    return sockfd;
}

void handle_chat(int socket_fd) {
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int n;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(socket_fd, &read_fds);

        if (select(socket_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            error("ERROR on select");
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(buffer, 0, BUFFER_SIZE);
            n = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
            if (n < 0) {
                error("ERROR reading from stdin");
            }
            n = write(socket_fd, buffer, strlen(buffer));
            if (n < 0) {
                error("ERROR writing to socket");
            }
        }

        if (FD_ISSET(socket_fd, &read_fds)) {
            memset(buffer, 0, BUFFER_SIZE);
            n = read(socket_fd, buffer, BUFFER_SIZE - 1);
            if (n < 0) {
                error("ERROR reading from socket");
            }
            printf("%s", buffer);
            fflush(stdout);
        }
    }
}

void debug_args(char *args[]) {
    int i = 0;
    printf("Arguments:\n");
    while (args[i] != NULL) {
        printf("args[%d]: %s\n", i, args[i]);
        i++;
    }
}

int main(int argc, char *argv[]) {
    char *exec_program = NULL;
    char *input_spec = NULL;
    char *output_spec = NULL;
    int opt, in_fd = -1, out_fd = -1;
    struct timespec start, end;
    double time_taken;

    while ((opt = getopt(argc, argv, "e:i:o:b:")) != -1) {
        switch (opt) {
            case 'e':
                exec_program = optarg;
                break;
            case 'i':
                input_spec = optarg;
                break;
            case 'o':
                output_spec = optarg;
                break;
            case 'b':
                input_spec = optarg;
                output_spec = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -e \"program args\" [-i input] [-o output] [-b both]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Special handling for both input and output
    if (input_spec != NULL && output_spec != NULL && strcmp(input_spec, output_spec) == 0) {
        if (strncmp(input_spec, "TCPS", 4) == 0) {
            int port = atoi(input_spec + 4);
            in_fd = start_tcp_server(port);
            out_fd = in_fd;
        } else {
            fprintf(stderr, "Invalid specification for both input and output\n");
            exit(EXIT_FAILURE);
        }
    } else {
        if (input_spec != NULL) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            if (strncmp(input_spec, "TCPS", 4) == 0) {
                int port = atoi(input_spec + 4);
                in_fd = start_tcp_server(port);
            } else if (strncmp(input_spec, "TCPC", 4) == 0) {
                char *hostname = strtok(input_spec + 4, ",");
                int port = atoi(strtok(NULL, ","));
                in_fd = start_tcp_client(hostname, port);
            } else {
                fprintf(stderr, "Invalid input specification\n");
                exit(EXIT_FAILURE);
            }
        }

        if (output_spec != NULL) {
            if (strncmp(output_spec, "TCPS", 4) == 0) {
                int port = atoi(output_spec + 4);
                out_fd = start_tcp_server(port);
            } else if (strncmp(output_spec, "TCPC", 4) == 0) {
                char *hostname = strtok(output_spec + 4, ",");
                int port = atoi(strtok(NULL, ","));
                out_fd = start_tcp_client(hostname, port);
            } else {
                fprintf(stderr, "Invalid output specification\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (exec_program == NULL) {
        int n= 100;
        char buffer[n];
        // No -e parameter, start chat mode
        if (in_fd != -1 && out_fd != -1){ // flag b
            close(STDIN_FILENO);
            dup(in_fd);
            close(STDOUT_FILENO);
            dup(out_fd);
            for(;;){
                n= read(0, buffer, n);
                buffer[n++]=0;
                write(1, buffer, n);
            }
        }
        if (in_fd != -1){
            close(STDIN_FILENO);
            dup(in_fd);
            for(;;){
                n= read(0, buffer, n);
                buffer[n++]=0;
                write(1, buffer, n);
            }
        }
        if(out_fd != -1){
            close(STDOUT_FILENO);
            dup(out_fd);
            for(;;){
                n= read(0, buffer, n);
                buffer[n++]=0;
                write(1, buffer, n);
            }
        }
        //     fprintf(stderr, "Error: No executable specified and input/output are not connected.\n");
        //     exit(EXIT_FAILURE);}
    }else {
        char *args[256];
        int i = 0;
        char *token = strtok(exec_program, " ");
        while (token != NULL && i < 255) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        debug_args(args);

        pid_t pid = fork();
        if (pid < 0) {
            error("ERROR on fork");
        } else if (pid == 0) {
            printf("Executing program: %s\n", exec_program);
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (out_fd != -1) {
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            printf("About to execute program\n"); // Added debug line
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else {
            if (in_fd != -1) close(in_fd);
            if (out_fd != -1) close(out_fd);
            wait(NULL);
            printf("Child process finished\n");
        }
    }

    return 0;
}

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

int main(int argc, char *argv[]) {
    char *exec_program = NULL;
    char *input_spec = NULL;
    char *output_spec = NULL;
    int opt, in_fd = -1, out_fd = -1;
    int flagb= 0;

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
                flagb=1;
                input_spec = optarg;
                output_spec = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -e \"program args\" [-i input] [-o output] [-b both]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (exec_program == NULL) {
        fprintf(stderr, "Usage: %s -e \"program args\" [-i input] [-o output] [-b both]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (input_spec != NULL && output_spec != NULL && strcmp(input_spec, output_spec) == 0 && flagb) {
         if (strncmp(input_spec, "TCPS", 4) == 0) {
            int port = atoi(input_spec + 4);
            in_fd = start_tcp_server(port);
            out_fd = in_fd;
        } else {
            fprintf(stderr, "Invalid specification for both input and output\n");
            exit(EXIT_FAILURE);
        }}
        

    if (input_spec != NULL && (!flagb)) {
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

    if (output_spec != NULL&& (!flagb)) {
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

    char *args[256];
    int i = 0;
    char *token = strtok(exec_program, " ");
    while (token != NULL && i < 255) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    pid_t pid = fork();
if (pid < 0) {
    error("ERROR on fork");
} else if (pid == 0) {
    printf("Executing program: %s\n", exec_program);
    if (in_fd != -1 && !flagb) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    if (out_fd != -1 && !flagb) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }
    if(in_fd != -1 && out_fd != -1 && flagb) {
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);  // Redirecting both input and output to the same TCP connection
        close(in_fd);
    }
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

    return 0;
}

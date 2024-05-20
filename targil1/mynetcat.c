#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s -e \"<program> <args>\" [-i TCPS<port>] [-o TCPC<ip,port>] [-b TCPS<port>]\n", progname);
    exit(EXIT_FAILURE);
}

int create_tcp_server(int port) {
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
   
        perror("bind");
        
        exit(EXIT_FAILURE);
    }
    printf("bind secc\n ");

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int accept_tcp_client(int sockfd) {
    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return newsockfd;
}

int create_tcp_client(const char *hostname, int port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if ((server = gethostbyname(hostname)) == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-e") != 0) {       
        usage(argv[0]);
    }
    char *cmd = argv[2];
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    int sockfd = -1, newsockfd = -1;
    int is_server = 0;
    int flag= 0;

    for (int i = 3; i < argc; i++) {            
        if (strncmp(argv[i], "-i", 2) == 0) {           
            flag=1;
            if (strncmp(argv[i] + 2, "TCPS", 4) == 0) {
                int port = atoi(argv[i] + 6);
                sockfd = create_tcp_server(port);
                input_fd = accept_tcp_client(sockfd);
                is_server = 1;
            }
        } else if (strncmp(argv[i], "-o", 2) == 0) {
            flag=1;
            if (strncmp(argv[i] + 2, "TCPC", 4) == 0) {
                char *ip_port = argv[i] + 6;
                char *comma = strchr(ip_port, ',');
                if (comma == NULL) {
                    printf("USAGE1\n");
                    usage(argv[0]);
                }
                *comma = '\0';
                char *ip = ip_port;
                int port = atoi(comma + 1);
                output_fd = create_tcp_client(ip, port);
            }
        } else if (strncmp(argv[i], "-b", 2) == 0) {
            flag=1;
            if (strncmp(argv[i] + 2, "TCPS", 4) == 0) {
                int port = atoi(argv[i] + 6);
                sockfd = create_tcp_server(port);
                input_fd = accept_tcp_client(sockfd);
                output_fd = input_fd;
                is_server = 1;
            }
        } else if(flag==0){
            printf("USAGE2\n");
            usage(argv[0]);
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Child process
        if (dup2(input_fd, STDIN_FILENO) == -1 ||
            dup2(output_fd, STDOUT_FILENO) == -1 ||
            dup2(output_fd, STDERR_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        char *args[] = {"/bin/sh", "-c", cmd, NULL};
        execvp(args[0], args);

        // If execvp returns, there was an error
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

        if (is_server && newsockfd != -1) {
            close(newsockfd);
        }

        if (sockfd != -1) {
            close(sockfd);
        }

        if (input_fd != STDIN_FILENO) {
            close(input_fd);
        }

        if (output_fd != STDOUT_FILENO) {
            close(output_fd);
        }
    }

    return 0;
}


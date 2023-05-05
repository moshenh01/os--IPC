#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>


#define BUFFER_SIZE 1024


int main(int argc, char *argv[]) {
    char *arg2 = argv[2]; 
    char *arg3 = argv[3];
    int port = atoi(arg3);
   

    int sockfd, n;
    struct sockaddr_in servaddr;
    char buffer[BUFFER_SIZE];
    struct pollfd fds[2];

    // Create a socket for the client
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Set up the server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, arg2, &servaddr.sin_addr);

    // Connect to the server
    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("Connected to the server.\n");

    // Set up the poll file descriptor for the client socket
    fds[0].fd = sockfd;
    fds[0].events = POLLIN ;
      // Set the O_NONBLOCK flag for stdin
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while (1) {
        // Use poll to wait for input
        int pull_count = poll(fds, 2, 2500);
        printf("poll_count: %d\n", pull_count);
        // Check for input on the socket file descriptor
        if (fds[0].revents & POLLIN) {
           // printf("fds[0].revents & POLLIN\n");
            
            n = recv(sockfd, buffer, BUFFER_SIZE, 0);
            //printf("0sockfd: %s", buffer);
            if (n == 0) {
                printf("The server closed the connection.\n");
                break;
            }
            printf("C Recv: %s", buffer);
        }
           // Check if the socket is ready for writing
        if (fds[0].revents & POLLOUT) {
            printf("fds[0].revents & POLLOUT\n");
            memset(buffer, 0, BUFFER_SIZE);
            n = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            printf("0stdin: %s", buffer);
            if (n > 0) {
                write(sockfd, buffer, strlen(buffer));
            }
             if (n == 0) {
                printf("The server closed the connection.\n");
                break;
            }
        }

       // Check for input on the stdin file descriptor
        if (fds[1].revents & POLLIN) {
            //printf("fds[1].revents & POLLIN\n");
            memset(buffer, 0, BUFFER_SIZE);
            int n = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            //printf("1stdin: %s", buffer);
             if (n > 0) {
                write(sockfd, buffer, strlen(buffer));
            }
           
        }
        
    }
  

    // Close the socket
    close(sockfd);

    return 0;
}

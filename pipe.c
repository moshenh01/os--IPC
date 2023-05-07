#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <sys/time.h>
#include "netcom.h"



#define BUFFER_SIZE 1024
#define DATA_SIZE 104857600 // 100MB
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8081
char time_str[20];
char *FIFO_FILE = NULL;
void calculate_md5_checksum(const char *data, size_t size, unsigned char *md5_checksum) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned int md_len;

    // Initialize the digest context
    md = EVP_md5();
    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);

    // Update the digest context with the data
    EVP_DigestUpdate(mdctx, data, size);

    // Finalize the digest and store the result in md5_checksum
    EVP_DigestFinal_ex(mdctx, md5_checksum, &md_len);
    //printf("md_len: %d\n", md_len);

    // Clean up the digest context
    EVP_MD_CTX_free(mdctx);
}


void pipe_sender(){
    char *data = (char *)malloc(DATA_SIZE * sizeof(char) + 1);
    if (data == NULL) {
    // handle error
        perror("malloc");
        exit(1);
    }
    
    unsigned char md5_checksum[MD5_DIGEST_LENGTH + 1];
    bzero(md5_checksum, MD5_DIGEST_LENGTH + 1);
    calculate_md5_checksum(data, DATA_SIZE, md5_checksum);
    md5_checksum[MD5_DIGEST_LENGTH] = '\0';

    int fd;
    // create the FIFO (named pipe)
    mkfifo(FIFO_FILE, 0666);
    // open the FIFO for writing
    fd = open(FIFO_FILE, O_WRONLY);

    write(fd, md5_checksum, MD5_DIGEST_LENGTH);
    sleep(1);
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int bytes_sent = write(fd, data, DATA_SIZE);
    if (bytes_sent < 0) {
        perror("write");
        exit(1);
    }
    printf("Data sent: %d\n", bytes_sent);
    
    // Calculate the elapsed time
    sprintf(time_str, "%ld", tv.tv_sec * 1000 + tv.tv_usec / 1000);
    close(fd);
    printf("FIFO closed.\n");
    unlink(FIFO_FILE);
    printf("FIFO unlinked.\n");


 
    free(data);
}

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


    int time_to_sent = 1;
    int send_pref = 0;

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

        if(time_to_sent == 1 && pull_count == 0){
            
            //memset(buffer, 0, BUFFER_SIZE);
            
            strcpy(buffer, "pipe");
            FIFO_FILE = argv[6];
            strcat(buffer, argv[6]);
            printf("buffer: %s\n", buffer);
            
            n = write(sockfd, buffer, strlen(buffer));
            
            if(n < 0){
                printf("Error sending data.\n");
                break;
            }
            

            sleep(2);//time for the servre
            printf("named pipe sender\n");
            pipe_sender();
           
            time_to_sent = 0;
            send_pref = 1;
        }
        if(send_pref == 1 && pull_count == 0){
            strcpy(buffer, "done_send");
            strcat(buffer, time_str);
            n = write(sockfd, buffer, strlen(buffer));
            //printf("n: %d\n", n);
            if(n < 0){
                printf("Error sending data.\n");
                break;
            }
            n =recv(sockfd, buffer, BUFFER_SIZE, 0);
           
            long long int time_in_ms = atof(time_str);
            long long int server_time_ms = atof(buffer);
            printf("time client: %lld\n", time_in_ms);
            printf("time server: %lld\n", server_time_ms);
            long long int diff = (server_time_ms - time_in_ms);
            
            printf("diff: %lld\n", diff);
            break;
            send_pref = 0;
        }

        
    }
  

    // Close the socket
    close(sockfd);

    return 0;
}

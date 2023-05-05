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



#define BUFFER_SIZE 1024
#define DATA_SIZE 104857600 // 100MB
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8081
char time_str[20];

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


void ipv4_tcp_sender(){
    



    int sockfd;
    struct sockaddr_in6 servaddr;
    //char buffer[BUFFER_SIZE];
    char *data = (char *)malloc(DATA_SIZE * sizeof(char) + 1);
    if (data == NULL) {
    // handle error
        perror("malloc");
        exit(1);
    }
    struct pollfd fdss[1];
   
    // Generate random data
    for (int i = 0; i < DATA_SIZE; i++) {
        //rand between A Z
        data[i] = 'A' + (rand() % 26);
    }
    data[DATA_SIZE] = '\0';
   
    
    unsigned char md5_checksum[MD5_DIGEST_LENGTH +1];
    //printf("MD5 checksum: %s  \n", md5_checksum);
    calculate_md5_checksum(data, DATA_SIZE, md5_checksum);
    md5_checksum[MD5_DIGEST_LENGTH] = '\0';
    //printf("MD5 checksum: %s \n", md5_checksum );

   
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    
    // Set up the server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(SERVER_PORT);
    inet_pton(AF_INET6, SERVER_IP, &servaddr.sin6_addr);
    // Connect to the server
    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("Connected to the server ipv4_tcp.\n");
     // Set up the poll file descriptor for the client socket
    fdss[0].fd = sockfd;
    fdss[0].events = POLLOUT;
    // Send the data and checksum to the server
    int bytes_sent = 0;
    int checksum_sent = 0;
    
    
    
    struct timeval start_time;

    gettimeofday(&start_time, NULL);
    while (bytes_sent < DATA_SIZE ) {
         poll(fdss, 1, -1);
        if (fdss[0].revents & POLLOUT) {
            int bytes_to_send = DATA_SIZE - bytes_sent;
            int bytes_sent_now;
            if(!checksum_sent) {
               
                int n = write(sockfd, md5_checksum, MD5_DIGEST_LENGTH);
                if(n > 0){
                    printf("checksum sent\n");
                    sleep(1);
                }
                checksum_sent = 1;
                bytes_sent_now =0;
            }
            else{
                bytes_sent_now = write(sockfd, data + bytes_sent, bytes_to_send);
                if (bytes_sent_now < 0) {
                    printf("Error sending data.\n");
                    break;
                }
            }
            bytes_sent += bytes_sent_now;
            
        }
       
    }
    sprintf(time_str, "%.5f", start_time.tv_sec + (double)start_time.tv_usec / 1000000);
    printf("2)bytes_sent: %d\n", bytes_sent);
    close(sockfd);
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
            
            strcpy(buffer, "ipv6_tcp");
            
            n = write(sockfd, buffer, strlen(buffer));
            //printf("n: %d\n", n);
            if(n < 0){
                printf("Error sending data.\n");
                break;
            }
            

            sleep(2);//time for the servre
            printf("ipv6_tcp_sender\n");
            ipv4_tcp_sender();
           
            time_to_sent = 0;
            send_pref = 1;
        }
        if(send_pref == 1 && pull_count == 0){
            strcpy(buffer, "done_send");
            n = write(sockfd, buffer, strlen(buffer));
            //printf("n: %d\n", n);
            if(n < 0){
                printf("Error sending data.\n");
                break;
            }
            n =recv(sockfd, buffer, BUFFER_SIZE, 0);
            //printf("server time: %s\nclien time: %s\n", buffer, time_str);
            double time_in_sec_float = atof(time_str);
            double server_time_float = atof(buffer);
            double diff = server_time_float - time_in_sec_float - 1;
            printf("diff: %.5f\n", diff);
            break;
            send_pref = 0;
        }

        
    }
  

    // Close the socket
    close(sockfd);

    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <sys/time.h>
#include "netcom.h"

#define PORT "8080"   // Port we're listening on
#define BUFFER_SIZE 64*1024 // 64 KUB
#define UDP_BUFFER_SIZE 32768
#define FILE_SIZE 100*1024*1024 // 100 MB
char *output_file = NULL;
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

    // Clean up the digest context
    EVP_MD_CTX_free(mdctx);
}

void ipv4_tcp_receiver(char *port){
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    char buffer[BUFFER_SIZE];
    FILE *fp;
    //off_t offset = 0;
   // char *file_name = "output.txt";
    char *data = (char *) malloc(FILE_SIZE * sizeof(char));
    if (data == NULL) {
    // handle error
        perror("malloc");
        exit(1);
    }

    // Create a socket for the server
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // Set up the server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(port));

    // Bind the socket to the server address
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 5);

    printf("Server listening on port %s...\n", port);
      connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    printf("Accepted connection from %s:%d.\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

       // Receive the checksum from the client
    printf("Receiving checksum from client...\n");
    char send_checksum[MD5_DIGEST_LENGTH+1];
    bzero(send_checksum,MD5_DIGEST_LENGTH+1);
    read(connfd, send_checksum, MD5_DIGEST_LENGTH);
    //printf("Checksum received.\n");
   // printf("Checksum: %s\n", send_checksum);
    

   

    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = 0;
    int sum = 0 ;
    fp = fopen("output.txt", "a+");
        if (fp == NULL) {
            perror("fopen");
            exit(1);
        }
    
    fseek(fp, 0, SEEK_SET);
    while ((bytes_received = read(connfd, buffer, BUFFER_SIZE)) > 0) {
        if (bytes_received < 0) {
            printf("Error receiving data \n");
            break;
        }
        // if(sum ==0)
        //     printf(":%c:%c:%c\n", buffer[0], buffer[1], buffer[2]);

        fwrite(buffer, sizeof(char), bytes_received, fp);
        
        sum += bytes_received;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    //copy file to data
    fseek(fp, 0, SEEK_SET);
    fread(data, sizeof(char), FILE_SIZE, fp);
    fclose(fp);

    if (sum < 100*1024*1024) {
        printf("data received is less than 100MB\n");
    } else {
        printf("data received %d is more than / =  100MB\n", sum);
    //     for (size_t i = 0; i < 24; i++)
    //     {
    //         printf(":%c", data[i]);
    //     }
    //     printf("\n");
    //     for (size_t i = 100*1024*1024-24; i < 100*1024*1024; i++)
    //     {
    //         printf(":%c", data[i]);
    //     }
    // printf("\n");
    
    }
    unsigned char our_checksum[MD5_DIGEST_LENGTH +1];
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
    //printf("Our checksum: %s\n", our_checksum);
    
    if (memcmp(our_checksum,send_checksum , MD5_DIGEST_LENGTH)==0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }
    
    close(listenfd);
    close(connfd);
    free(data);
}

void ipv4_udp_receiver(char *port) {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    char buffer[UDP_BUFFER_SIZE ];
    FILE *fp;

    // Create a socket for the server
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Set up the server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(port));

    // Bind the socket to the server address
    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("Server listening on port %s...\n", port);

    // Receive the checksum from the client
    printf("Receiving checksum from client...\n");
    char send_checksum[MD5_DIGEST_LENGTH +1];

    bzero(send_checksum, MD5_DIGEST_LENGTH+1);
    recvfrom(sockfd, send_checksum, MD5_DIGEST_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);
    printf("Checksum received.\n");
    //printf("Checksum: %s\n", send_checksum);

    memset(buffer, 0, UDP_BUFFER_SIZE );
    int bytes_received = 0;
    int sum = 0;
    fp = fopen("output.txt", "a+");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }
    int buffer_size = 1024*1024; // set buffer size to 1MB
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }
    while (sum < FILE_SIZE) {
        bytes_received = recvfrom(sockfd, buffer,UDP_BUFFER_SIZE  , 0, (struct sockaddr *)&cliaddr, &len);
        if (bytes_received < 0) {
            printf("Error receiving data \n");
            break;
        }

        fwrite(buffer, sizeof(char), bytes_received, fp);
       

        sum += bytes_received;
       /// printf("sum = %d\n", sum);
    }

    struct timeval tv;
   
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    //copy file to data
    
    char *data = (char *)malloc(FILE_SIZE * sizeof(char));
    if (data == NULL) {
        // handle error
        perror("malloc");
        exit(1);
    }
    fseek(fp, 0, SEEK_SET);
    fread(data, sizeof(char), FILE_SIZE, fp);
    fclose(fp);

    if (sum < 100 * 1024 * 1024) {
        printf("data received is less than 100MB\n");
    } else {
        printf("data received %d is more than / =  100MB\n", sum);
        //for print the last 24 bytes
       
        
    }

    unsigned char our_checksum[MD5_DIGEST_LENGTH + 1];
    bzero(our_checksum, MD5_DIGEST_LENGTH + 1);
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
   // printf("Our checksum: %s\n", our_checksum);

    if(memcmp(our_checksum, send_checksum,MD5_DIGEST_LENGTH) == 0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }

    free(data);
    close(sockfd);
}

void ipv6_tcp_receiver(char *port){
    int listenfd, connfd;
    struct sockaddr_in6 servaddr, cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    char buffer[BUFFER_SIZE];
    FILE *fp;
    
  
    char *data = (char *) malloc(FILE_SIZE * sizeof(char));
    if (data == NULL) {
    // handle error
        perror("malloc");
        exit(1);
    }

    // Create a socket for the server
    listenfd = socket(AF_INET6, SOCK_STREAM, 0);

    // Set up the server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(atoi(port));

    // Bind the socket to the server address
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }
    listen(listenfd, 5);

    printf("Server listening on port %s...\n", port);
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    char cliaddr_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &cliaddr.sin6_addr, cliaddr_str, INET6_ADDRSTRLEN);
    printf("Accepted connection from %s:%d.\n", cliaddr_str, ntohs(cliaddr.sin6_port));

       // Receive the checksum from the client
    printf("Receiving checksum from client...\n");
    char send_checksum[MD5_DIGEST_LENGTH+1];
    bzero(send_checksum,MD5_DIGEST_LENGTH+1);
    read(connfd, send_checksum, MD5_DIGEST_LENGTH);
    //printf("Checksum received.\n");
   // printf("Checksum: %s\n", send_checksum);
    

   

    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = 0;
    int sum = 0 ;
    fp = fopen("output.txt", "a+");
        if (fp == NULL) {
            perror("fopen");
            exit(1);
        }
    
    fseek(fp, 0, SEEK_SET);
    while ((bytes_received = read(connfd, buffer, BUFFER_SIZE)) > 0) {
        if (bytes_received < 0) {
            printf("Error receiving data \n");
            break;
        }
        // if(sum ==0)
        //     printf(":%c:%c:%c\n", buffer[0], buffer[1], buffer[2]);

        fwrite(buffer, sizeof(char), bytes_received, fp);
        
        sum += bytes_received;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    //copy file to data
    fseek(fp, 0, SEEK_SET);
    fread(data, sizeof(char), FILE_SIZE, fp);
    fclose(fp);

    if (sum < 100*1024*1024) {
        printf("data received is less than 100MB\n");
    } else {
        printf("data received %d is more than / =  100MB\n", sum);
    //     for (size_t i = 0; i < 24; i++)
    //     {
    //         printf(":%c", data[i]);
    //     }
    //     printf("\n");
    //     for (size_t i = 100*1024*1024-24; i < 100*1024*1024; i++)
    //     {
    //         printf(":%c", data[i]);
    //     }
    // printf("\n");
    
    }
    unsigned char our_checksum[MD5_DIGEST_LENGTH +1];
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
    //printf("Our checksum: %s\n", our_checksum);
    
    if (memcmp(our_checksum,send_checksum , MD5_DIGEST_LENGTH)==0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }
    
    close(listenfd);
    close(connfd);
    free(data);  
}

void ipv6_udp_receiver(char *port){
    int sockfd;
     struct sockaddr_in6 servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    char buffer[UDP_BUFFER_SIZE ];
    FILE *fp;

    // Create a socket for the server
    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

    // Set up the server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(atoi(port));

    // Bind the socket to the server address
    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("Server listening on port %s...\n", port);

    // Receive the checksum from the client
    printf("Receiving checksum from client...\n");
    char send_checksum[MD5_DIGEST_LENGTH +1];

    bzero(send_checksum, MD5_DIGEST_LENGTH+1);
    recvfrom(sockfd, send_checksum, MD5_DIGEST_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);
    printf("Checksum received.\n");
    //printf("Checksum: %s\n", send_checksum);

    memset(buffer, 0, UDP_BUFFER_SIZE );
    int bytes_received = 0;
    int sum = 0;
    fp = fopen("output.txt", "a+");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }
    int buffer_size = 1024*1024; // set buffer size to 1MB
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }
    while (sum < FILE_SIZE) {
        bytes_received = recvfrom(sockfd, buffer,UDP_BUFFER_SIZE  , 0, (struct sockaddr *)&cliaddr, &len);
        if (bytes_received < 0) {
            printf("Error receiving data \n");
            break;
        }

        fwrite(buffer, sizeof(char), bytes_received, fp);
       

        sum += bytes_received;
       /// printf("sum = %d\n", sum);
    }

    struct timeval tv;
   
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    //copy file to data
    
    char *data = (char *)malloc(FILE_SIZE * sizeof(char));
    if (data == NULL) {
        // handle error
        perror("malloc");
        exit(1);
    }
    fseek(fp, 0, SEEK_SET);
    fread(data, sizeof(char), FILE_SIZE, fp);
    fclose(fp);

    if (sum < 100 * 1024 * 1024) {
        printf("data received is less than 100MB\n");
    } else {
        printf("data received %d is more than / =  100MB\n", sum);
        //for print the last 24 bytes
       
        
    }

    unsigned char our_checksum[MD5_DIGEST_LENGTH + 1];
    bzero(our_checksum, MD5_DIGEST_LENGTH + 1);
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
   // printf("Our checksum: %s\n", our_checksum);

    if(memcmp(our_checksum, send_checksum,MD5_DIGEST_LENGTH) == 0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }

    free(data);
    close(sockfd);
}

void uds_dgram_receiver(){
    int sockfd;
    struct sockaddr_un servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    char *buffer = (char *)malloc(FILE_SIZE * sizeof(char));
    if (buffer == NULL) {
        // handle error
        perror("malloc");
        exit(1);
    }
    FILE *fp;

    // Create a socket for the server
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);

    // Set up the server address
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path,SOCKET_PATH,sizeof(servaddr.sun_path) - 1);
    printf("servaddr.sun_path: %s\n", servaddr.sun_path);
    // Bind the socket to the server address
    
    unlink(SOCKET_PATH); // remove any existing socket file
    
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("bind failed");
        exit(1);
    }
    printf("socket is at -->%s\n", SOCKET_PATH);

    // Receive the checksum from the client
    printf("Receiving checksum from client...\n");
    char send_checksum[MD5_DIGEST_LENGTH +1];

    bzero(send_checksum, MD5_DIGEST_LENGTH+1);
    recvfrom(sockfd, send_checksum, MD5_DIGEST_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);
    printf("Checksum received.\n");
    //printf("Checksum: %s\n", send_checksum);

    memset(buffer, 0, BUF_UDS);
    int bytes_received = 0;
    int sum = 0;
    fp = fopen("output.txt", "a+");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }
    // int buffer_size = 1024*1024*100; // set buffer size to 1MB
    // if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
    //     perror("setsockopt failed");
    //     exit(1);
    // }
    while (sum < FILE_SIZE) {
        bytes_received = read(sockfd, buffer, BUF_UDS);
        if (bytes_received < 0) {
            printf("Error receiving data \n");
            break;
        }

        fwrite(buffer, sizeof(char), bytes_received, fp);
       

        sum += bytes_received;
        //printf("sum = %d\n", sum);
    }

    struct timeval tv;
   
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    //copy file to data
    
    char *data = (char *)malloc(FILE_SIZE * sizeof(char));
    if (data == NULL) {
        // handle error
        perror("malloc");
        exit(1);
    }
    fseek(fp, 0, SEEK_SET);
    fread(data, sizeof(char), FILE_SIZE, fp);
    fclose(fp);

    if (sum < 100 * 1024 * 1024) {
        printf("data received is less than 100MB\n");
    } else {
        printf("data received %d is more than / =  100MB\n", sum);
        //for print the last 24 bytes
       
        
    }

    unsigned char our_checksum[MD5_DIGEST_LENGTH + 1];
    bzero(our_checksum, MD5_DIGEST_LENGTH + 1);
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
   // printf("Our checksum: %s\n", our_checksum);

    if(memcmp(our_checksum, send_checksum,MD5_DIGEST_LENGTH) == 0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }

    free(data);
    free(buffer);
    close(sockfd);
}

void uds_stream_receiver(){
    int listenfd, connfd;
    struct sockaddr_un servaddr, cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    char buffer[BUFFER_SIZE];
    FILE *fp;
  
    char *data = (char *) malloc(FILE_SIZE * sizeof(char));
    if (data == NULL) {
    // handle error
        perror("malloc");
        exit(1);
    }

    // Create a socket for the server
    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);

    // Set up the server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path,SOCKET_PATH , sizeof(servaddr.sun_path) - 1);

    
    // Bind the socket to the server address
    unlink(SOCKET_PATH); // remove any existing socket file
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 5);

    printf("Server listening on path %s...\n", SOCKET_PATH);
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    if (connfd < 0) {
        perror("accept");
        exit(1);
    }

       // Receive the checksum from the client
    printf("Receiving checksum from client...\n");
    char send_checksum[MD5_DIGEST_LENGTH+1];
    bzero(send_checksum,MD5_DIGEST_LENGTH+1);
    read(connfd, send_checksum, MD5_DIGEST_LENGTH);
    //printf("Checksum received.\n");
   // printf("Checksum: %s\n", send_checksum);
    

   

    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = 0;
    int sum = 0 ;
    fp = fopen("output.txt", "a+");
        if (fp == NULL) {
            perror("fopen");
            exit(1);
        }
    
    fseek(fp, 0, SEEK_SET);
    while ((bytes_received = read(connfd, buffer, BUFFER_SIZE)) > 0) {
        if (bytes_received < 0) {
            printf("Error receiving data \n");
            break;
        }
        // if(sum ==0)
        //     printf(":%c:%c:%c\n", buffer[0], buffer[1], buffer[2]);

        fwrite(buffer, sizeof(char), bytes_received, fp);
        
        sum += bytes_received;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    //copy file to data
    fseek(fp, 0, SEEK_SET);
    fread(data, sizeof(char), FILE_SIZE, fp);
    fclose(fp);

    if (sum < 100*1024*1024) {
        printf("data received is less than 100MB\n");
    } else {
        printf("data received %d is more than / =  100MB\n", sum);
    
    }
    unsigned char our_checksum[MD5_DIGEST_LENGTH +1];
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
    //printf("Our checksum: %s\n", our_checksum);
    
    if (memcmp(our_checksum,send_checksum , MD5_DIGEST_LENGTH)==0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }
    
    close(listenfd);
    close(connfd);
    free(data);
}

void mmap_receiver(){
     // Open the file for writing
    int fd;
    while(output_file == NULL){
        sleep(0.1);
        continue;
    }
    printf("output file is %s\n", output_file);
    fd = open(output_file, O_RDWR | O_CREAT , (mode_t)0600);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    if (lseek(fd, FILE_SIZE - 1, SEEK_SET) == -1) {
        perror("lseek");
        exit(1);
    }
      if (write(fd, "", 1) == -1) {
        perror("write");
        exit(1);
    }
    // Map the file to memory
    char *mapped = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    // Wait for the sender to write to the memory-mapped file
    printf("Waiting for sender to write to memory-mapped file...\n");
    while (mapped[0] == '\0') {
        sleep(1);
    }
    char send_checksum[MD5_DIGEST_LENGTH+1];
    if(mapped[16] == '\0'){
        printf("recieves chacksum\n");
        
        bzero(send_checksum,MD5_DIGEST_LENGTH+1);
        memcpy(send_checksum, mapped, MD5_DIGEST_LENGTH);
        printf("Checksum: %s\n", send_checksum);
        memset(mapped, 0, MD5_DIGEST_LENGTH);

    }
    while (mapped[0] == '\0') {
        sleep(0.0001);
    }
    while(mapped[FILE_SIZE-1] != 'I'){
        sleep(0.0001);
    }
   

  
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    // for(int i = FILE_SIZE-5;i < FILE_SIZE; i++){
    //     printf(":%c",mapped[i]);
    // }
    // printf("\n");
    char *data = malloc(FILE_SIZE + 1);
    if (data == NULL) {
        perror("malloc");
        exit(1);
    }
    memcpy(data, mapped, FILE_SIZE);
    data[FILE_SIZE] = '\0';
    // for(int i = FILE_SIZE-5;i < FILE_SIZE; i++){
    //     printf(":%c",data[i]);
    // }
    // printf("\n");
    unsigned char our_checksum[MD5_DIGEST_LENGTH +1];
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
    printf("Our checksum: %s\n", our_checksum);
    
    if (memcmp(our_checksum,send_checksum , MD5_DIGEST_LENGTH)==0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }
  

    // Unmap the file and close the file descriptor
    if (munmap(mapped, FILE_SIZE) == -1) {
        perror("munmap");
    }
    if (close(fd) == -1) {
        perror("close");
    }
    free(data);
}

void pipe_receiver(){
    int fd;
    char *data = (char *)malloc(FILE_SIZE * sizeof(char) + 1);
    if (data == NULL) {
    // handle error
        perror("malloc");
        exit(1);
    }

    mkfifo(FIFO_FILE, 0666);
    printf("Waiting for sender...\n");
    fd = open(FIFO_FILE, O_RDONLY);

    char send_checksum[MD5_DIGEST_LENGTH+1];
    bzero(send_checksum,MD5_DIGEST_LENGTH+1);
    if(read(fd, send_checksum, MD5_DIGEST_LENGTH) < 0){
        perror("read");
        exit(1);
    }
    //printf("Checksum: %s\n", send_checksum);
    memset(data, 0, FILE_SIZE);
    int total_bytes_read = 0;
    while(total_bytes_read < FILE_SIZE){
        int bytes_read = read(fd, data + total_bytes_read, FILE_SIZE - total_bytes_read);
        if(bytes_read < 0){
            perror("read");
            exit(1);
        }
        total_bytes_read += bytes_read;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(time_str, "%.5f", tv.tv_sec + (double)tv.tv_usec / 1000000);
    
    printf("Received %d bytes\n",total_bytes_read);

    unsigned char our_checksum[MD5_DIGEST_LENGTH +1];
    calculate_md5_checksum(data, FILE_SIZE, our_checksum);
    //printf("Our checksum: %s\n", our_checksum);

    if (memcmp(our_checksum,send_checksum , MD5_DIGEST_LENGTH)==0) {
        printf("Checksums match!\n");
    } else {
        printf("Checksums do not match!\n");
    }
    close(fd);
    unlink(FIFO_FILE);
    printf("FIFO closed.\n");
    free(data);
    printf("FIFO unlinked.\n");
}

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(char *port)
{
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    int rv;
    

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

// Main
int main(int argc, char *argv[])
{
    int listener;     // Listening socket descriptor
    char *arg2 = argv[2];
    
    int newfd;        // Newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;

    char buf[256];    // Buffer for client data

    char remoteIP[INET6_ADDRSTRLEN];

    // Start off with room for 5 connections
    // (We'll realloc as necessary)
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    // Set up and get a listening socket
    listener = get_listener_socket(arg2);

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    // Add the listener to set
    pfds[0].fd = listener;
    pfds[0].events = POLLIN; // Report ready to read on incoming connection

    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;
    fd_count = 2; // For the listener and stdin
    int sender_fd;
    // Main loop
    for(;;) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }
        printf("poll_count = %d\n", poll_count);
        // Run through the existing connections looking for data to read
        for(int i = 0; i < fd_count; i++) {

            // Check if someone's ready to read
            if (pfds[i].revents & POLLIN && (i != 1)) { // We got one!!

                if (pfds[i].fd == listener) {
                    // If listener is ready to read, handle new connection

                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);
                    sender_fd = newfd;
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

                        printf("pollserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // If not the listener, we're just a regular client
                    memset(buf, 0, sizeof buf);
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

                    sender_fd = pfds[i].fd;

                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        } else {
                            perror("recv");
                        }

                        close(pfds[i].fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);

                    } else {
                        // We got some good data from a client
                        printf("S received: %s", buf);
                        

                        if(strcmp(buf,"ipv4_tcp") == 0){
                            printf("\nopening ipv4_tcp\n");
                            arg2 = "8081";
                            ipv4_tcp_receiver(arg2);
                        }
                        else if(strcmp(buf,"ipv4_udp") == 0){
                            printf("\nopening ipv4_udp\n");
                            arg2 = "8081";
                            ipv4_udp_receiver(arg2);
                            
                        }
                        else if(strcmp(buf,"ipv6_tcp") == 0){
                            printf("\nopening ipv6_tcp\n");
                            arg2 = "8081";
                            ipv6_tcp_receiver(arg2);
                        }
                         else if(strcmp(buf,"ipv6_udp") == 0){
                            printf("\nopening ipv6_udp\n");
                            arg2 = "8081";
                            ipv6_udp_receiver(arg2);
                        }
                         else if(strcmp(buf,"uds_dgram") == 0){
                            printf("\nopening uds_dgram\n");
                            arg2 = "8081";
                            uds_dgram_receiver();
                        }
                         else if(strcmp(buf,"uds_stream") == 0){
                            printf("\nopening uds_stream\n");
                            arg2 = "8081";
                            uds_stream_receiver();
                        }
                         else if(strncmp(buf,"mmap",4) == 0){
                            printf("\nopening mmap\n");
                            arg2 = "8081";
                            output_file = malloc(sizeof(buf));
                            strcpy(output_file, buf + 4);
                            
                            mmap_receiver();
                            free(output_file);
                        }
                         else if(strncmp(buf,"pipe",4) == 0){
                            printf("\nopening pipe\n");
                            arg2 = "8081";
                            FIFO_FILE = malloc(sizeof(buf));
                            strcpy(FIFO_FILE, buf + 4);
                            //printf("\noutput file: %s\n", FIFO_FILE);
                            pipe_receiver();
                        }
    
                        else if((strcmp(buf, "done_send") == 0)){
                                strcpy(buf, time_str);
                                send(sender_fd, buf, sizeof(buf), 0);
                                printf("\nsending time: %s\n", buf);
                        }
                        memset(buf, 0, sizeof buf);
                       

                 
                    }
                } // END handle data from client
            } // END got ready-to-read from poll()
            if((pfds[i].revents & POLLIN) && (i == 1)) {
                //printf("POLLOUT\n");
                memset(buf, 0, sizeof buf);
                read(STDIN_FILENO, buf, sizeof buf);
                
                send(sender_fd, buf, sizeof(buf), 0);
                //printf("S sent: %s", buf);
                memset(buf, 0, sizeof buf);
                
            }

        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    free(pfds);
    
    return 0;
}
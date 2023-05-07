#include "netcom.h"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define DATA_SIZE 104857600 // 100MB

char time_str[20];
char *output_file = NULL;

void calculate_md5_checksum(const char *data, size_t size, unsigned char *md5_checksum)
{
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
    // printf("md_len: %d\n", md_len);

    // Clean up the digest context
    EVP_MD_CTX_free(mdctx);
}

void mmap_sender()
{

    char *data = (char *)malloc(DATA_SIZE * sizeof(char) + 1 + 16);
    if (data == NULL)
    {
        // handle error
        perror("malloc");
        exit(1);
    }
    // Generate random data
    for (int i = 0; i < DATA_SIZE; i++)
    {
        // rand between A Z
        data[i] = 'A' + (rand() % 26);
    }
    data[DATA_SIZE -1] = 'I';
    data[DATA_SIZE] = '\0';

    unsigned char md5_checksum[MD5_DIGEST_LENGTH + 1];
    // printf("MD5 checksum: %s  \n", md5_checksum);
    calculate_md5_checksum(data, DATA_SIZE, md5_checksum);
    md5_checksum[MD5_DIGEST_LENGTH] = '\0';
    //printf("MD5 checksum: %s \n", md5_checksum );

    // Open the output file
    int output_fd = open(output_file, O_RDWR | O_CREAT | O_TRUNC, FILE_MODE);
    if (output_fd < 0)
    {
        perror("open output file");
        exit(1);
    }
    // Truncate the output file to the desired size (100MB)
    if (ftruncate(output_fd, (DATA_SIZE )) == -1)
    {
        perror("ftruncate");
        exit(1);
    }
    printf("Open the output file\n");

    // Memory-map the output file
    char *output_data = mmap(NULL, (DATA_SIZE ), PROT_READ | PROT_WRITE, MAP_SHARED, output_fd, 0);
    if (output_data == MAP_FAILED)
    {
        perror("mmap output file");
        close(output_fd);
        exit(1);
    }
    if (output_data == NULL)
    {
        printf("output_data is NULL\n");
    }
    printf("Copy the data to the output file\n");
    // Copy the data to the output file
    // char *ptr = &data[0];
    // int bytes_resived = 0;

    printf("start while\n");
    // while(*ptr != '\0'){
    memcpy(output_data, md5_checksum, MD5_DIGEST_LENGTH); 
    printf("send checksum\n");
    sleep(1);
    memset(output_data, 0, MD5_DIGEST_LENGTH);

    struct timeval tv;
    gettimeofday(&tv, NULL);
  

    if (memcpy(output_data, data, DATA_SIZE ) == NULL)
    {
        printf("memcpy error\n");
    }

    //     ptr += 102400;
    //     output_data += 102400;
    //     bytes_resived += 102400;
    // }
    // printf("data resived: %d\n",bytes_resived);

    printf("Synchronize changes to the output file\n");

    // Synchronize changes to the output file
    if (msync(output_data, DATA_SIZE , MS_SYNC) < 0)
    {
        perror("msync");

        close(output_fd);

        munmap(output_data, DATA_SIZE );
        exit(1);
    }
    // Calculate the elapsed time
    sprintf(time_str, "%ld", tv.tv_sec * 1000 + tv.tv_usec / 1000);
    munmap(output_data, DATA_SIZE );
    close(output_fd);
    free(data);
}

int main(int argc, char *argv[])
{
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
    fds[0].events = POLLIN;
    // Set the O_NONBLOCK flag for stdin
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    int time_to_sent = 1;
    int send_pref = 0;

    while (1)
    {
        // Use poll to wait for input
        int pull_count = poll(fds, 2, 2500);
        printf("poll_count: %d\n", pull_count);
        // Check for input on the socket file descriptor
        if (fds[0].revents & POLLIN)
        {
            // printf("fds[0].revents & POLLIN\n");

            n = recv(sockfd, buffer, BUFFER_SIZE, 0);
            // printf("0sockfd: %s", buffer);
            if (n == 0)
            {
                printf("The server closed the connection.\n");
                break;
            }
            printf("C Recv: %s", buffer);
        }

        // Check for input on the stdin file descriptor
        if (fds[1].revents & POLLIN)
        {
            // printf("fds[1].revents & POLLIN\n");
            memset(buffer, 0, BUFFER_SIZE);
            int n = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            // printf("1stdin: %s", buffer);
            if (n > 0)
            {
                write(sockfd, buffer, strlen(buffer));
            }
        }

        if (time_to_sent == 1 && pull_count == 0)
        {

            strcpy(buffer, "mmap");

            output_file = (char *)malloc(strlen(argv[6]) * sizeof(char) + 1);
            if (output_file == NULL)
            {
                // handle error
                perror("malloc");
                exit(1);
            }
            strcpy(output_file, argv[6]);
            strcat(buffer, output_file);
            n = write(sockfd, buffer, strlen(buffer));

            if (n < 0)
            {
                printf("Error sending data.\n");
                break;
            }

            sleep(1); // time for the servre
            printf("mmap_sender\n");
            mmap_sender();
            free(output_file);

            time_to_sent = 0;
            send_pref = 1;
        }
        if (send_pref == 1 && pull_count == 0)
        {
            strcpy(buffer, "done_send");
            strcat(buffer, time_str);
            n = write(sockfd, buffer, strlen(buffer));
            // printf("n: %d\n", n);
            if (n < 0)
            {
                printf("Error sending data.\n");
                break;
            }
            n = recv(sockfd, buffer, BUFFER_SIZE, 0);
            // printf("server time: %s\nclien time: %s\n", buffer, time_str);
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

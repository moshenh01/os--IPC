#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/wait.h>

#define MAX_BUFFER_SIZE 1024
#define TEST_DATA_SIZE 100 * 1024 * 1024



int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [-c IP PORT | -s PORT]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

 /* Extend your tool, to make it a network performance test utility.
    You can use your chat channel, for system communication, like transferring states, times , etc.
    By performance test we mean the next task.
    1) Generate a chunk of data, with 100MB size
    2) Generate a checksum for the data above
    3) Transmit the data with selected communication style, while measuring the time it takes
    4) Report the result to stdOut
    communications styles are:
    tcp/udp ipv4/ipv6 (4 variants)
    mmap a file. Named pipe (2 variants)
    Unix Domain Socket (UDS) :stream and datagram (2 variants)
    usage:
    The client side: stnc -c IP PORT -p <type> <param>
    -p will indicate to perform the test
    <type> will be the communication types: so it can be ipv4,ipv6,mmap,pipe,uds
    <param> will be a parameter for the type. It can be udp/tcp or dgram/stream or file name:*/
    if(argv[4] != NULL && strcmp(argv[4],"-p") == 0){
        
        if (argc < 6) {
            printf("Usage: %s -c IP PORT -p <type> <param>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        if(strcmp(argv[5],"ipv4") == 0){
            if(strcmp(argv[6],"tcp") == 0){
                printf("executing ipv4_tcp\n");
                argv[0] = "./ipv4_tcp";

            }
            else if(strcmp(argv[6],"udp") == 0){
                printf("executing ipv4_udp\n");
                argv[0] = "./ipv4_udp";
            }
        }
        else if(strcmp(argv[5],"ipv6") == 0){
            if(strcmp(argv[6],"tcp") == 0){
                printf("executing ipv6_tcp\n");
                argv[0] = "./ipv6_tcp";
            }
            else if(strcmp(argv[6],"udp") == 0){
                printf("executing ipv6_udp\n");
                argv[0] = "./ipv6_udp";
            }
        }
        else if(strcmp(argv[5],"mmap") == 0){
            printf("executing mmap\n");
            argv[0] = "./mmap";
        }
        else if(strcmp(argv[5],"pipe") == 0){
            printf("executing pipe\n");
            argv[0] = "./pipe";
        }
        else if(strcmp(argv[5],"uds") == 0){
            if(strcmp(argv[6],"dgram") == 0){
                printf("executing uds_dgram\n");
                argv[0] = "./uds_dgram";
            }
            else if(strcmp(argv[6],"stream") == 0){
                printf("executing uds_stream\n");
                argv[0] = "./uds_stream";
            }
        }
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        if(pid == 0){

            

            if(execvp(argv[0],argv) == -1){
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }

        }
        else{
            int status;
            waitpid(pid,&status,0);
            printf("child exited with status %d\n",status);
        }
        exit(0);
        
            
    }

    else if(strcmp(argv[1], "-c") == 0) { // Client mode
        if (argc < 4) {
            printf("Usage: %s -c IP PORT\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        argv[0] = "./client";
        pid_t pid = fork();
        if (pid == -1)
	    {
            perror("fork failed");
            exit(EXIT_FAILURE);
	    }
        if(pid == 0){

            

            if(execvp(argv[0],argv) == -1){
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }

        }
        else{
            int status;
            waitpid(pid,&status,0);
            printf("child exited with status %d\n",status);
        }
        exit(0);

    }

    else if (strcmp(argv[1], "-s") == 0) { // Server mode
        if (argc < 3) {
            printf("Usage: %s -s PORT\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        argv[0] = "./server";
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        if(pid == 0){

            

            if(execvp(argv[0],argv) == -1){
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }

        }
        else{
            int status;
            waitpid(pid,&status,0);
            printf("child exited with status %d\n",status);
        }
        exit(0);
    }
   
   
    
}
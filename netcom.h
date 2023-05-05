#ifndef NETCOM_H
#define NETCOM_H

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
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


#define SOCKET_PATH "/home/moshe/mysocket"

#define BUF_UDS 1024*1024*100
#define FILE_MODE 0644
extern char *input_file;
extern char *output_file;


#endif

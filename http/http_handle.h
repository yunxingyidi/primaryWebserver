#ifndef HTTPHADDLE_H
#define HTTPHADDLE_H
//http连接处理
//

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../threadlocker/locker.h"
class http_handle
{
private:
    /* data */
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    http_handle(/* args */);
    ~http_handle();
};

http_handle::http_handle(/* args */)
{
    
}

http_handle::~http_handle()
{
}

#endif HTTPHADDLE_H
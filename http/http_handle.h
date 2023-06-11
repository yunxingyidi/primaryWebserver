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
    //请求方法
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE
    };
    http_handle(){};
    ~http_handle(){};
    //epoll实例，管理文件描述符
    static int m_epollfd;
    
    //用户连接数
    static int m_user_count;
    int m_sockfd;
    sockaddr_in m_address;
    // 存储读取的请求报文数据
    char m_read_buf[READ_BUFFER_SIZE];
    // 缓冲区中m_read_buf中数据的最后一个字节的下一个位置
    int m_read_idx;
    // m_read_buf读取的位置m_checked_idx
    int m_checked_idx;
    // m_read_buf中已经解析的字符个数
    int m_start_line;
    // 存储发出的响应报文数据
    char m_write_buf[WRITE_BUFFER_SIZE];
    // 指示buffer中的长度
    int m_write_idx;


};

#endif HTTPHADDLE_H
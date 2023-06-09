#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
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

#include "../threadlocker/locker.h"
#include "../time/timer.h"
#include "../log/log.h"
/* Http处理类
 * 2023-6-9
 * 将从socket中读出的信息按照http报文的格式进行解析，
 * 得到请求行，请求头部，和请求数据，再根据其状态变化
 * 进行响应报文的制作
*/
#define FILENAME_LEN 2048
#define READ_BUFFER_SIZE 2048
#define WRITE_BUFFER_SIZE 1024
class http_handle
{
public:
    // HTTP方法名
    enum METHOD
    {
        GET = 0,
        POST
    };
    // 解析报文进度
    enum CHECK_STATE
    {
        REQUESTLINE_STATE = 0,
        HEADER_STATE,
        CONTENT_STATE
    };
    // HTTP状态
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    // 从状态机的状态，文本解析是否成功
    enum LINE
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_handle() {}
    ~http_handle() {}
    // 初始化套接字
    void init(int sockfd, const sockaddr_in &addr, char *, int, int);
    void process();
    // 读取浏览器发送的数据
    bool read();
    // 给相应报文中写入数据
    bool write();
    // 关闭HTTP连接
    void close_conn(bool real_close = true);
    // http处理函数
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    int timer_flag;
    // 是否正在处理数据中 
    int isrunning;  
    void init();
    // 从m_read_buf读取，并处理请求报文
    HTTP_CODE http_read();
    // 向m_write_buf写入响应报文数据
    bool http_write(HTTP_CODE ret);
    // 主状态机解析报文中的请求行数据
    HTTP_CODE do_request_line(char *text);
    // 主状态机解析报文中的请求头数据
    HTTP_CODE do_headers(char *text);
    // 主状态机解析报文中的请求内容
    HTTP_CODE do_content(char *text);
    // 生成响应报文
    HTTP_CODE do_request();
    // m_start_line是已经解析的字符
    // get_line用于将指针向后偏移，指向未处理的字符
    char *get_line() { return m_read_buf + m_start_line; };
    // 从状态机读取一行，分析是请求报文的哪一部分
    LINE parse_line();
    // 之后重点介绍**
    void unmap();
    // 根据响应报文格式，生成对应8个部分，以下函数均由do_request调用
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    static int m_epollfd;
    static int m_user_count;
    // IO事件读为0, 写为1
    int m_state; 
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
    // 主状态机的状态
    CHECK_STATE m_check_state;
    // 请求方法
    METHOD m_method;
    // 以下为解析请求报文中对应的6个变量
    // 存储读取文件的名称
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;
    // 读取服务器上的文件地址
    char *m_file_address;
    struct stat m_file_stat;
    // io向量机制iovec
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        // 是否启用的POST
    char *m_string; // 存储请求头数据
                    // 剩余发送字节数
    int bytes_to_send;
    // 已发送字节数
    int bytes_have_send;
    char *m_root;

    int m_trig_mode;              // 触发模式
    int m_log;             // 是否开启日志
};

#endif

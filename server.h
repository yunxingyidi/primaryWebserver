#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http_handle.h"
#include "./log/log.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位
/* 服务器类 
 * 2023-5-15
 * 完成一个socket服务器的基本功能，包括对于事件的监听，对于超时连接的处理，
 * 作为主循环程序对于连接请求和后续处理进行相应的调度
*/
class Server
{
public:
    Server();
    ~Server();
    //初始化函数，对于端口号和服务器进行配置
    void init(int port, int log_write, int opt_linger, int trig_mode, int thread_num, int log, int actor_model);
    //初始化线程池
    void thread_pool();
    void log_init();
    void trig_mode();
    void socket_monitor();
    void Loop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealwithnewconn();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础信息
    int m_port;//端口
    char *m_root;//根目录
    int m_log_write;//日志类型
    int m_log;//是否启动日志
    int m_actormodel;//Reactor/Proactor
    int m_OPT_LINGER;
    //网络信息
    int m_pipefd[2];//相互连接的套接字
    int m_epollfd;//epoll对象
    http_handle *users;//单个http连接

    //线程池相关
    threadpool<http_handle> *m_pool;
    int m_thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];
    int m_listenfd;//监听套接字
    int m_trig_mode;//ET/LT
    int m_listen_trig_mode;//ET/LT
    int m_con_trig_mode;//ET/LT

    //定时器相关
    client_data *users_timer;
    //工具类
    Handle handle;
};
#endif

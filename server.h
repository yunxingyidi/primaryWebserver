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

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class Server
{
public:
    Server();
    ~Server();

    void init(int port, int log_write, int trigmode,int thread_num, int close_log, int actor_model);
    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclinetdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础信息
    int m_port;//端口
    char *m_root;//根目录
    int m_log_write;//日志类型
    int m_close_log;//是否启动日志
    int m_actormodel;//Reactor/Proactor
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
    int m_TRIGMode;//ET/LT
    int m_LISTENTrigmode;//ET/LT
    int m_CONNTrigmode;//ET/LT

    //定时器相关
    client_data *users_timer;
    //工具类
    Handle handle;
};
#endif

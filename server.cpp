#include "server.h"

//主要完成服务器初始化：http连接、设置根目录、开启定时器对象
Server::Server()
{
    //http_handle最大连接数
    users = new http_handle[MAX_FD];
    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);
    //定时器，创建储存定时器
    users_timer = new client_data[MAX_FD];
}
//服务器资源释放
Server::~Server()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}
//初始化用户名、数据库等信息
void Server::init(int port, int log_write, int opt_linger, int trig_mode, int thread_num, int log, int actor_model)
{
    m_port = port;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_trig_mode = trig_mode;
    m_OPT_LINGER = opt_linger;
    m_log = log;
    m_actormodel = actor_model;
}

//设置epoll的触发模式：ET、LT，包括连接和监听两种
void Server::trig_mode()
{
    //LT + LT
    if (0 == m_trig_mode)
    {
        m_listen_trig_mode = 0;
        m_con_trig_mode = 0;
    }
    //LT + ET
    else if (1 == m_trig_mode)
    {
        m_listen_trig_mode = 0;
        m_con_trig_mode = 1;
    }
    //ET + LT
    else if (2 == m_trig_mode)
    {
        m_listen_trig_mode = 1;
        m_con_trig_mode = 0;
    }
    //ET + ET
    else if (3 == m_trig_mode)
    {
        m_listen_trig_mode = 1;
        m_con_trig_mode = 1;
    }
}
//初始化日志系统
void Server::log_init()
{
    if (0 == m_log)
    {
        //确定日志类型：同步/异步
        if (1 == m_log_write)
            Log::get_instance()->init("./ServerLog", m_log, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", m_log, 2000, 800000, 0);
    }
}

//创建线程池
void Server::thread_pool()
{
    //线程池
    m_pool = new threadpool<http_handle>(m_actormodel, m_thread_num);
}

//服务器段开启一个socket进行监听，主要对m_listenfd进行操作。
void Server::socket_monitor()
{
    //SOCK_STREAM 表示使用面向字节流的TCP协议，IPV4
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    //匹配socket的close行为
    struct linger tmp = {0, 1};
    //设置套接自选项
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    int ret = 0;
    struct sockaddr_in address;
    //结构体清0
    bzero(&address, sizeof(address));
    //设置IPV4
    address.sin_family = AF_INET;
    //sin_addr.s_addr 字段设置为 htonl(INADDR_ANY)，表示服务器将接受任意来源的连接请求。
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    //设置端口号
    address.sin_port = htons(m_port);
    //sin_port 字段使用 htons() 函数将端口号 m_port 进行网络字节序的转换。
    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    //使用 bind() 函数将套接字绑定到 address 所指定的地址。
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    //表示已连接和未连接的最大队列数总和为10
    ret = listen(m_listenfd, 10);
    //设置服务器的最小时间间隙
    handle.init(TIMESLOT);
    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    //新建epoll，最多处理10个事件
    m_epollfd = epoll_create(10);

    handle.addfd(m_epollfd, m_listenfd, false, m_listen_trig_mode);
    http_handle::m_epollfd = m_epollfd;
    //socketpair()函数用于创建一对无名的、相互连接的套接子,此部分用于全双工通信。
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    handle.setnonblocking(m_pipefd[1]);
    handle.addfd(m_epollfd, m_pipefd[0], false, 0);

    handle.addsig(SIGPIPE, SIG_IGN);
    handle.addsig(SIGALRM, handle.sig_handler, false);
    handle.addsig(SIGTERM, handle.sig_handler, false);

    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Handle::u_pipefd = m_pipefd;
    Handle::u_epollfd = m_epollfd;
}

//创建一个定时器节点，将连接信息挂载
void Server::timer(int connfd, struct sockaddr_in client_address)
{
    //建立一个http事件处理
    users[connfd].init(connfd, client_address, m_root, m_con_trig_mode, m_log);
    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    //TIMESLOT:最小时间间隔单位为5s
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    handle.m_timer_lst.add_timer(timer);
}

//若数据活跃，则将定时器节点往后延迟3个时间单位
//并对新的定时器在链表上的位置进行调整
void Server::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    handle.m_timer_lst.adjust_timer(timer);
    LOG_INFO("%s", "adjust timer once");
}

//删除定时器节点，关闭连接
void Server::deal_timer(util_timer *timer, int sockfd)
{
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        handle.m_timer_lst.del_timer(timer);
    }

    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

//http 处理用户数据
bool Server::dealwithnewconn()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    //LT水平触发
    if (0 == m_listen_trig_mode)
    {
        //对socket进行接受
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        //判断不合法connfd
        if (connfd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (http_handle::m_user_count >= MAX_FD)
        {
            handle.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);
    }
    // ET边缘触发
    else
    {
        //边缘触发需要一直accept直到为空
        while (1)
        {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_handle::m_user_count >= MAX_FD)
            {
                handle.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

//处理定时器信号,set the timeout ture
bool Server::dealwithsignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    //从管道读端读出信号值，成功返回字节数，失败返回-1
    //正常情况下，这里的ret返回值总是1，只有14和15两个ASCII码对应的字符
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        // handle the error
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        //处理信号值对应的逻辑
        for (int i = 0; i < ret; ++i)
        {
            
            //这里面明明是字符
            switch (signals[i])
            {
            //这里是整型
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            //关闭服务器
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

//处理客户连接上接收到的数据
void Server::dealwithread(int sockfd)
{
    //创建定时器临时变量，将该连接对应的定时器取出来
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (1 == m_actormodel)
    {
        if (timer)
        {
            //将定时器往后延迟3个单位
            adjust_timer(timer);
        }

        //若监测到读事件，将该事件放入请求队列
        m_pool->append(users + sockfd, false);
        while (true)
        {
            //是否正在处理中
            if (1 == users[sockfd].improv)
            {
                //事件类型关闭连接
                if (1 == users[sockfd].timer_flag)
                {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    //proactor
    else
    {   
        //先读取数据，再放进请求队列
        if (users[sockfd].read())
        {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            //将该事件放入请求队列
            m_pool->append_to(users + sockfd);
            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            deal_timer(timer, sockfd);
        }
    }
}

//写操作
void Server::dealwithwrite(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (1 == m_actormodel)
    {
        if (timer)
        {
            adjust_timer(timer);
        }

        m_pool->append(users + sockfd, true);

        while (true)
        {
            if (1 == users[sockfd].improv)
            {
                if (1 == users[sockfd].timer_flag)
                {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (users[sockfd].write())
        {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            deal_timer(timer, sockfd);
        }
    }
}

//事件回环（即服务器主线程）
void Server::Loop()
{
    bool timeout = false;
    bool stop_server = false;
    //不停止一直运行
    while (!stop_server)
    {
        //等待所监控文件描述符上有事件的产生
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        //返回请求数目
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }
        //对所有就绪事件进行处理
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            //如果接受到的是当前的客户建立连接
            if (sockfd == m_listenfd)
            {
                bool flag = dealwithnewconn();
                if (false == flag)
                    continue;
            }
            //当events为EPOLLRDHUP | EPOLLHUP | EPOLLERR，处理异常事件
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //处理定时器信号
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                //接收到SIGALRM信号，timeout设置为True
                bool flag = dealwithsignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            //处理客户连接上send的数据
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }

        //处理定时器为非必须事件，收到信号并不是立马处理
        //完成读写事件后，再进行处理
        if (timeout)
        {
            handle.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}
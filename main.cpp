#include "config.h"
//服务器主程序，实现Web服务器
int main(int argc, char *argv[])
{
    //初始化命令
    Config config;
    config.get_arg(argc, argv);
    Server server;
    //初始化web服务器
    server.init(config.PORT, 0, 0, config.trig_mode, 
                config.thread_num, config.log, config.actor_model);
    //初始化日志功能
    server.log_init();
    //初始化线程池线程池
    server.thread_pool();
    //选择触发模式
    server.trig_mode();
    //开启对于socket的监听
    server.socket_monitor();
    //进入socket主线程运行
    server.Loop();
    return 0;
}

#ifndef CONFIG_H
#define CONFIG_H
/* 服务器配置类
 * 2023.5.15
 * 初始化服务器基本配置，或进行参数选择
*/
#include "server.h"
using namespace std;

class Config
{
public:
    Config();
    ~Config(){};
    //获取输入
    void get_arg(int argc, char*argv[]);
    //端口号
    int PORT;
    //触发组合模式
    int trig_mode;
    //监听时触发模式
    int listen_trig_mode;
    //连接时触发模式
    int con_trig_mode;
    //线程池内的线程数量
    int thread_num;
    //是否关闭日志
    int log;
    //并发模型选择
    int actor_model;
};

#endif
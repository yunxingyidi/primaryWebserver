#include "config.h"

Config::Config(){
    //端口号,默认9006
    PORT = 9006;
    //默认listenfd LT + connfd LT
    trig_mode = 0;
    //默认水平触发
    listen_trig_mode = 0;
    //默认水平触发
    con_trig_mode = 0;
    //线程池内的线程数量,默认10
    thread_num = 10;
    //关闭日志,默认不关闭
    log = 0;
    //并发模型,默认是proactor
    actor_model = 0;
}

void Config::get_arg(int argc, char*argv[]){
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            PORT = atoi(optarg);
            break;
        }
        case 't':
        {
            trig_mode = atoi(optarg);
            break;
        }
        case 'n':
        {
            thread_num = atoi(optarg);
            break;
        }
        case 'l':
        {
            log = atoi(optarg);
            break;
        }
        case 'm':
        {
            actor_model = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}

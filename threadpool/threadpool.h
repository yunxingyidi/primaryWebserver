#ifndef THREADPOOL_H
#define THREADPOOL_H
//线程池
//请求队列：保存监听线程提供的http连接信息
//工作线程：处理http请求

#include <iostream>
#include <pthread.h>
#include <list>
#include "../threadlocker/locker.h"
using namespace std;

template <typename T>
class threadpool
{
public:
    threadpool(int act_model, int max_thread = 8, int max_queue = 10000);
    ~threadpool();
    bool append_to(T* request);
    bool append(T *request, bool wen);
private:
    //线程数量
    int thread_num;
    //请求数量
    int request_num;
    //线程数组
    pthread_t *threads;
    //先入先出请求队列
    std::list<T*> request_queue;
    //共享资源控制
    locker queue_locker;
    sem queue_sem;
    int actor_model;
    static void* work(void* arg);
    void handle_request();
};
template <typename T>
threadpool<T>::threadpool(int act_model, int max_thread, int max_queue)
{
    threads = new pthread_t[max_thread];
    if(!threads)
    {
        throw std::exception();
    }
    //逐个创建线程
    for(int i = 0; i < max_thread; i++)
    {
        int flag = pthread_create(threads + i, NULL, work, this);
        if(flag)
        {
            delete []threads;
            throw std::exception();
        }
        flag = pthread_detach(threads[i]);
        if(flag)
        {
            delete []threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] threads;
}
template <typename T>
bool threadpool<T>::append_to(T* request)
{
    queue_locker.lock();
    if(request_queue.size() >= request_num)
    {
        queue_locker.unlock();
        return false;
    }
    request_queue.push_back(request);
    queue_sem.post();
    queue_locker.unlock();
    return true;
}
//reactor模式下的请求入队
template <typename T>
bool threadpool<T>::append(T *request, bool wen)
{
    queue_locker.lock();
    if (request_queue.size() >= request_num)
    {
        queue_locker.unlock();
        return false;
    }
    //读写事件
    if(wen)
        request->m_state = 1;
    else
        request->m_state = 0;
    request_queue.push_back(request);
    queue_locker.unlock();
    queue_sem.post();
    return true;
}
template <typename T>
//线程工作函数，在创建时开始运行，在其中不断调用请求处理函数函数run
void* threadpool<T>::work(void* arg)
{
    threadpool *p = (threadpool*)arg;
    while(true)
        p->handle_request();
    return p;
}
template <typename T>
//请求处理函数,取得队头请求消息，进行如处理程序
void threadpool<T>::handle_request()
{
    // queue_sem.wait();
    // queue_locker.lock();
    // if(request_queue.size() <= 0)
    // {
    //     queue_locker.unlock();
    //     return;
    // }
    // T* run_request = request_queue.front();
    // request_queue.pop_front();
    // queue_locker.unlock();
    // if(!run_request)
    // {
    //     return;
    // } 
    while (true)
    {
        queue_sem.wait();
        queue_locker.lock();
        if (request_queue.empty())
        {
            queue_locker.unlock();
            continue;
        }
        //
        T *request = request_queue.front();
        request_queue.pop_front();
        queue_locker.unlock();
        if (!request)
            continue;
        //Reactor
        if (1 == actor_model)
        {
            //IO事件类型：0为读
            if (0 == request->m_state)
            {
                if (request->read())
                {
                    request->improv = 1;
                    request->process();
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        //default:Proactor，线程池不需要进行数据读取，而是直接开始业务处理
        //之前的操作已经将数据读取到http的read和write的buffer中了
        else
        {
            request->process();
        }
    } 
}
#endif
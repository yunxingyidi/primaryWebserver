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
    bool append(T* request);
private:
    int thread_num;
    int request_num;
    pthread_t *threads;
    std::list<T*> request_queue;
    locker queue_locker;
    sem queue_sem;
    int actor_model;
    static void* work(void* arg);
    void run()
};
template <typename T>
threadpool<T>::threadpool(int act_model, int max_thread, int max_queue)
{
    threads = new pthread_t[max_thread];
    if(!threads)
    {
        throw std::exception();
    }
    for(int i = 0; i < max_thread; i++)
    {
        int flag = pthread_create(threads[i], NULL, work, this);
        if(flag)
        {
            delete []threads;
            throw std::exception()
        }
        flag = pthread_detach(thread[i]);
        if(flag)
        {
            delete []threads;
            throw std::exception()
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] threads;
}
template <typename T>
bool threadpool<T>::append(T* request)
{
    queue_locker.lock()
    if(request_queue.size() >= request_num)
    {
        queue_locker.unlock()
        return false;
    }
    request_queue.push_back(request);
    queue_sem.post()
    queue_locker.unlocker();
    return true;
}
template <typename T>
void* threadpool<T>::work(void* arg)
{
    threadpool *p = (threadpool*)arg;
    while(true)
        p->run();
    return pool;
}
template <typename T>
void threadpool<T>::run()
{
    queue_sem.wait()
    queue_locker.lock();
    if(request_queue.size() <= 0)
    {
        queue_locker.unlock();
        return;
    }
    T* run_request = request_queue.front();
    request_queue.pop_front();
    queue_locker.unlock();
    if(!run_request)
    {
        return;
    }
    
}
#endif
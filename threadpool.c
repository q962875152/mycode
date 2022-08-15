#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LL_ADD(item, list) do { \
    item->prev = NULL;          \
    item->next = list;          \
    if (list != NULL) list->prev = item; \
    list = item;    \
} while (0)

#define LL_REMOVE(item, list) do { \
    if (item->prev != NULL) item->prev->next = item->next; \
    if (item->next != NULL) item->next->prev = item->prev; \
    if (item == list)   list = item->next; \
    item->prev = item->next = NULL; \
} while(0)

typedef struct NWORKER {

    pthread_t id;
    int terminate;

    struct NTHREADPOOL *pool;

    struct NWORKER *prev;
    struct NWORKER *next;
} nworker;

typedef struct NJOB {

    void (*job_func)(void *arg);
    void *user_data;

    struct NJOB *prev;
    struct NJOB *next;
    
} njob;

typedef struct NTHREADPOOL {

    nworker *workers;
    njob *wait_jobs;

    pthread_cond_t cond;//线程间同步
    pthread_mutex_t mtx;//互斥锁
} nthreadpool;

void *thread_callback(void *arg) {
    nworker *worker = (nworker *)arg;

    while (1) {

        pthread_mutex_lock(&worker->pool->mtx);
        while (worker->pool->wait_jobs == NULL) {
            if (worker->terminate) break;
            pthread_cond_wait(&worker->pool->cond, &worker->pool->mtx);//此函数进入函数体后，会先对互斥锁解锁，然后在退出函数体后，再对互斥锁加锁。
                                                                        //阻塞等待一个条件变量(等待唤醒)并解除相应的锁资源
        }

        if (worker->terminate) {
            pthread_mutex_unlock(&worker->pool->mtx);
            break;
        }

        njob *job = worker->pool->wait_jobs;
        if (job != NULL) {
            LL_REMOVE(job, worker->pool->wait_jobs);
        }

        pthread_mutex_unlock(&worker->pool->mtx);

        if (job == NULL) continue;
        job->job_func(job);

    }

    free(worker);
}

int thread_pool_create(nthreadpool *pool, int num_thread) {

    if (pool == NULL) return -1;
    if (num_thread < 1) num_thread = 1;
    memset(pool, 0, sizeof(nthreadpool));

    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;//初始化
    memcpy(&pool->cond, &blank_cond, sizeof(pthread_cond_t));//如何初始化非标准变量，最好用memcpy
    pthread_mutex_t blank_mtx = PTHREAD_MUTEX_INITIALIZER;//这个初始化变量代表它是普通锁
    memcpy(&pool->mtx, &blank_mtx, sizeof(pthread_mutex_t));

    int idx = 0;
    for (idx = 0; idx < num_thread; idx++) {
        nworker *worker = (nworker *)malloc(sizeof(nworker));
        if (worker == NULL) {
            perror("malloc");//先输出函数参数中的字符串，后面再加上错误的原因。它将这些输出到标准错误输出设备。标准错误输出设备默认是终端。
            return idx;
        }
        memset(worker, 0, sizeof(nworker));

        worker->pool = pool;

        int ret = pthread_create(&worker->id, NULL, thread_callback, worker);
        if (ret) {
            perror("pthread_create");
            free(worker);
            return idx;
        }
        LL_ADD(worker, pool->workers);
    }

    return idx;

}

int thread_pool_destroy(nthreadpool *pool) {
    nworker *worker = NULL;
    for (worker = pool->workers; worker != NULL; worker = worker->next) worker->terminate = 1;
    pthread_mutex_lock(&pool->mtx);
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mtx);
}

int thread_pool_push_job(nthreadpool *pool, njob *job) {
    pthread_mutex_lock(&pool->mtx);
    LL_ADD(job, pool->wait_jobs);
    pthread_cond_signal(&pool->cond);//如果signal没人接；发送一个信号给另外一个正在处于阻塞等待状态的线程,
                                    //使其脱离阻塞状态,继续执行. 如果没有线程处在阻塞等待状态,pthread_cond_signal也会成功返回
    pthread_mutex_unlock(&pool->mtx);
}

#if 1

void counter(void *arg) {
    njob *job = (njob *)arg;
    if (job == NULL) return;

    int idx = *(int *)job->user_data;
    printf("idx=%d,selfid=%lu\n", idx, pthread_self()); //获得线程自身的ID

    free(job->user_data);
    free(job);
}

int main(int argc, char **argv) {
    nthreadpool pool = {0};
    int num_thread = 50;

    thread_pool_create(&pool, num_thread);
    for (int i = 0; i < 1000; i++) {
        njob *job = (njob*)malloc(sizeof(njob));
        memset(job, 0, sizeof(njob));

        job->job_func = counter;
        job->user_data = malloc(sizeof(int));//防止作用域返回栈栈上数据被回收
        *(int *)job->user_data = i;

        thread_pool_push_job(&pool, job);
    }

    getchar();

    return 0;
}

#endif
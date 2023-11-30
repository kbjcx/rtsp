#ifndef RTSPSERVER_THREADPOOL_H
#define RTSPSERVER_THREADPOOL_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

#include "Thread.h"

class ThreadPool {
public:
    class Task {
    public:
        using TaskCallback = void (*)(void*);
        Task()
            : mTaskCallback(nullptr)
            , mArg(nullptr) {}

        void setTaskCallback(TaskCallback cb, void* arg) {
            mTaskCallback = cb;
            mArg = arg;
        }

        void handle() {
            if (mTaskCallback) {
                mTaskCallback(mArg);
            }
        }

        void operator=(const Task& task) {
            this->mTaskCallback = task.mTaskCallback;
            this->mArg = task.mArg;
        }

    private:
        TaskCallback mTaskCallback;
        void* mArg;
    };

    static ThreadPool* createNew(int num);

    explicit ThreadPool(int num);
    ~ThreadPool();

    void addTask(Task& task);

private:
    void loop();

    class MThread : public Thread {
    protected:
        virtual void run(void* arg);
    };

    void createThreads();
    void cancelThreads();

private:
    std::queue<Task> mTaskQueue;
    std::mutex mMutex;
    std::condition_variable mCondVal;

    std::vector<MThread> mThreads;

    bool mQuit;
};

#endif

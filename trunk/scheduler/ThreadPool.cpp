#include "ThreadPool.h"

#include <mutex>

#include "../base/log.h"

ThreadPool* ThreadPool::createNew(int num) {
    return new ThreadPool(num);
}

ThreadPool::ThreadPool(int num)
    : mThreads(num)
    , mQuit(false) {
    createThreads();
}

ThreadPool::~ThreadPool() {
    cancelThreads();
}

void ThreadPool::addTask(ThreadPool::Task& task) {
    std::unique_lock<std::mutex> lck(mMutex);
    mTaskQueue.push(task);
    mCondVal.notify_one();
}

void ThreadPool::loop() {
    while (!mQuit) {
        std::unique_lock<std::mutex> lck(mMutex);
        if (mTaskQueue.empty()) {
            mCondVal.wait(lck);
        }

        if (mTaskQueue.empty()) {
            continue;
        }

        Task task = mTaskQueue.front();
        mTaskQueue.pop();
        task.handle();
    }
}

void ThreadPool::createThreads() {
    std::unique_lock<std::mutex> lck(mMutex);
    for (auto& thread : mThreads) {
        thread.start(this);
    }
}

void ThreadPool::cancelThreads() {
    std::unique_lock<std::mutex> lck(mMutex);
    mQuit = true;
    mCondVal.notify_all();
    for (auto& thread : mThreads) {
        thread.join();
    }

    mThreads.clear();
}

void ThreadPool::MThread::run(void* arg) {
    ThreadPool* threadPool = (ThreadPool*)arg;
    threadPool->loop();
}

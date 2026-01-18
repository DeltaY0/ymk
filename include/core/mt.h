#pragma once

#include <defines.h>

#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>

namespace ymk {

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t threads = 0) : stop(false) {
        // Auto-detect thread count
        if (threads == 0) threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 4; // Safety fallback

        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { 
                            return this->stop || !this->tasks.empty(); 
                        });

                        if(this->stop && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    void wait_all() {
        // TODO: nothin
        return;
    }

    template<class F>
    void add_task(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.push(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker : workers) {
            if(worker.joinable())
                worker.join();
        }
    }
};

} // namespace ymk
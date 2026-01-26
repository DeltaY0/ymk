#pragma once

#include <defines.h>

#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace ymk {

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    
    std::condition_variable wait_cv; 
    std::atomic<int> working_count{0}; // Tracks active tasks
    bool stop;

public:
    ThreadPool(size_t threads = 0) : stop(false) {
        if (threads == 0) threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 4;

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

                    // Task Finished
                    working_count--;
                    wait_cv.notify_all(); // Wake up the main thread if it's waiting
                }
            });
        }
    }

    template<class F>
    void add_task(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.push(std::forward<F>(f));
            working_count++; // <--- Track new task
        }
        condition.notify_one();
    }

    void wait_idle() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        // Wait until queue is empty AND no threads are working
        wait_cv.wait(lock, [this] { 
            return this->tasks.empty() && working_count == 0; 
        });
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
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

struct ThreadPool {
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;

    explicit ThreadPool(size_t n) {
        for (size_t i = 0; i < n; ++i)
            threads.emplace_back([this]{
                while (true) {
                    std::unique_lock lk(mtx);
                    cv.wait(lk, [&]{ return !tasks.empty() || stop; });
                    if (stop && tasks.empty()) return;
                    auto f = std::move(tasks.front()); tasks.pop();
                    f();
                }
            });
    }
    void submit(std::function<void()> f) {
        { std::lock_guard lk(mtx); tasks.push(std::move(f)); }
        cv.notify_one();
    }
    ~ThreadPool() {
        stop = true; cv.notify_all();
        for (auto& t : threads) t.join();
    }
};

ThreadPool& pool();
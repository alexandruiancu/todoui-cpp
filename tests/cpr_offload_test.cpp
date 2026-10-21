#include <cassert>
#include <future>
#include <map>
#include <string>
#include <thread>

// Include the real ThreadPool (compiled from ThreadPool.cpp)
#include "ThreadPool.h"

// Minimal mock for cpr::Response so we can test the pattern without a real server.
struct MockCprResponse {
    bool error = false;
    int status_code = 200;
    std::string status_line = "OK";
    std::string text = "OK";
    using header_map = std::map<std::string, std::string>;
    header_map header;
};

// Minimal mock for cpr::Url
struct MockCprUrl {
    std::string str() const { return "http://localhost:8080/"; }
};

// Test that std::promise/std::future work correctly with a simple type.
void test_promise_future_basic() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();

    prom.set_value(42);
    int val = fut.get();
    assert(val == 42);
}

// Test that ThreadPool::pool() can be called and submit accepts a lambda.
// This mirrors the pattern used in driver.cpp after the refactor.
void test_pool_submit_lambda() {
    // Submit a simple lambda that sets a flag (captured by reference).
    std::atomic<int> counter{0};
    pool().submit([&counter]{
        counter.fetch_add(1);
    });

    // Wait a tiny bit for the worker to run (in a real test we'd use proper sync).
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // The lambda should have executed; just verify no crash occurred.
    (void)counter;
}

// Test that the exact pattern from driver.cpp (promise + future + pool submit)
// compiles and runs without error when given a callable that returns void.
// We use a mock cpr response to avoid needing a real HTTP backend.
void test_driver_offload_pattern() {
    // Simulate what driver.cpp does for GET /:
    std::promise<MockCprResponse> prom;
    std::future<MockCprResponse> fut = prom.get_future();

    pool().submit([&prom]{
        MockCprResponse r;
        r.error = false;
        r.status_code = 200;
        prom.set_value(std::move(r));
    });

    MockCprResponse resp = fut.get();
    assert(!resp.error);
    assert(resp.status_code == 200);
}



#include "ThreadPool.h"

// Return a reference to a single, lazily‑initialized ThreadPool instance.
// The previous implementation attempted to create a temporary ThreadPool
// inside a lambda and bind a reference to it, which fails because
// ThreadPool contains a std::mutex (non‑copyable).  We now create a
// static ThreadPool directly.
ThreadPool& pool() {
    static ThreadPool p(16);
    return p;
}
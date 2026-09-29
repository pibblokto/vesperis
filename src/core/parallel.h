// Row-band parallelism for the per-pixel passes (M10-08) and the band-parallel drawing (B-310). Work is split into
// contiguous ranges so results stay identical to the serial loop. Since B-310 the workers are a persistent pool: a
// frame dispatches a dozen parallel regions (sky, terrain, flora, the mush passes, the RGB conversion), and creating
// threads for each cost more than a millisecond per frame.
#pragma once
#include <thread>
#include <vector>
#include <algorithm>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <cstdlib>

inline int parallelThreads() {
    static int n = 0;
    if (!n) {
        n = (int)std::thread::hardware_concurrency();
        if (const char* e = std::getenv("VESPERIS_THREADS")) n = std::atoi(e);   // B-310: for the bench (VESPERIS_THREADS=1 is the serial baseline)
        if (n < 1) n = 1; if (n > 8) n = 8;
    }
    return n;
}

class WorkerPool {
public:
    static WorkerPool& instance() { static WorkerPool p; return p; }
    // Runs job(i) for i in 1..count-1 on the workers while the caller runs job(0); returns when all are done.
    // Not reentrant: a job must not dispatch.
    void run(int count, const std::function<void(int)>& job) {
        if (count <= 1) { job(0); return; }
        start(count - 1);
        {
            std::lock_guard<std::mutex> lock(m);
            cur = &job; curCount = count; pending = count - 1; generation++;
        }
        cv.notify_all();
        job(0);
        std::unique_lock<std::mutex> lock(m);
        done.wait(lock, [&] { return pending == 0; });
        cur = nullptr;
    }
    ~WorkerPool() {
        { std::lock_guard<std::mutex> lock(m); quit = true; }
        cv.notify_all();
        for (auto& t : workers) t.join();
    }
private:
    void start(int n) {
        std::lock_guard<std::mutex> lock(m);
        while ((int)workers.size() < n) {
            int idx = (int)workers.size() + 1;
            workers.emplace_back([this, idx] { loop(idx); });
        }
    }
    void loop(int idx) {
        long seen = 0;
        for (;;) {
            const std::function<void(int)>* job = nullptr;
            {
                std::unique_lock<std::mutex> lock(m);
                cv.wait(lock, [&] { return quit || (generation != seen && cur && idx < curCount); });
                if (quit) return;
                seen = generation;
                job = cur;
            }
            (*job)(idx);
            {
                std::lock_guard<std::mutex> lock(m);
                if (--pending == 0) done.notify_one();
            }
        }
    }
    std::vector<std::thread> workers;
    std::mutex m;
    std::condition_variable cv, done;
    const std::function<void(int)>* cur = nullptr;
    int curCount = 0, pending = 0;
    long generation = 0;
    bool quit = false;
};

// Calls fn(begin, end) over [0, n) in bands; runs serially when the work is small.
template <class F>
void parallelFor(int n, int minPerThread, const F& fn) {
    int threads = std::min(parallelThreads(), std::max(1, n / std::max(1, minPerThread)));
    if (threads <= 1) { fn(0, n); return; }
    int band = (n + threads - 1) / threads;
    std::function<void(int)> job = [&](int t) {
        int b = t * band, e = std::min(n, b + band);
        if (b < e) fn(b, e);
    };
    WorkerPool::instance().run(threads, job);
}

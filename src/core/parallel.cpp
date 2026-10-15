// F-06: the worker pool behind `parallelFor` and `parallelBands` (parallel.h).
//
// A dispatch publishes its body, opens it and moves `gen` on, waking the sleeping workers; a worker that sees `gen` move counts itself
// `active`, enters the body if it is still open (and the dispatch wants it: its index under the count) and leaves. The caller runs the
// body too from the start (a worker's wake through the kernel takes 10-30 us: the caller takes chunks meanwhile), closes it and waits
// for `active` to fall to zero. The close and the entry are the two sides of one store-and-load pair (sequentially consistent): either
// the caller sees the worker counted and waits for it, or the worker sees the body closed and never touches it. The body's fields are
// written only while no worker is active, before the open that publishes them. The workers sleep between jobs: kept awake for 50 us
// after each (spinning, or in the M4's low-power wait), they won 0.15 ms on the surface at 1x-3x and lost 0.1-0.4 on the space frames
#include "parallel.h"
#include <cstdint>

namespace {

class Pool {
public:
    bool dispatch(int count, void (*fn)(void*), void* ctx) {
        bool& in = pool::inside();
        if (count <= 1 || in) { const bool was = in; in = true; fn(ctx); in = was; return true; }
        if (!busy.try_lock()) { contendedN++; return false; }
        in = true;
        start(count - 1);
        jobFn = fn; jobCtx = ctx; jobCount = count;
        open.store(true);
        {
            std::lock_guard<std::mutex> lock(m);
            gen.fetch_add(1);
        }
        wake.notify_all();
        fn(ctx);
        open.store(false);
        if (active.load() != 0) {
            std::unique_lock<std::mutex> lock(m);
            done.wait(lock, [&] { return active.load() == 0; });
        }
        in = false;
        busy.unlock();
        return true;
    }
    long contended() const { return contendedN.load(); }
    ~Pool() {
        {
            std::lock_guard<std::mutex> lock(m);
            quit = true;
            gen.fetch_add(1);
        }
        wake.notify_all();
        for (std::thread& t : workers) t.join();
    }

private:
    void start(int n) {
        while ((int)workers.size() < n) {
            const int idx = (int)workers.size() + 1;
            const uint64_t seen = gen.load();
            workers.emplace_back([this, idx, seen] { loop(idx, seen); });
        }
    }
    void loop(int idx, uint64_t seen) {
        pool::inside() = true;
        for (;;) {
            {
                std::unique_lock<std::mutex> lock(m);
                wake.wait(lock, [&] { return gen.load() != seen; });
                seen = gen.load();
                if (quit) return;
            }
            active.fetch_add(1);
            if (open.load() && idx < jobCount) jobFn(jobCtx);
            if (active.fetch_sub(1) == 1) { std::lock_guard<std::mutex> lock(m); done.notify_one(); }
        }
    }
    std::mutex busy;                     // one dispatch at a time (a second caller runs alone)
    std::mutex m;                        // the sleepers'
    std::condition_variable wake, done;  // the workers', the caller's (for its last workers)
    std::vector<std::thread> workers;    // workers[i] has the index i + 1 (the caller is 0)
    std::atomic<uint64_t> gen{0};
    std::atomic<bool> open{false};
    std::atomic<int> active{0};
    std::atomic<long> contendedN{0};
    bool quit = false;                   // under m
    void (*jobFn)(void*) = nullptr;
    void* jobCtx = nullptr;
    int jobCount = 0;
};

Pool& thePool() { static Pool p; return p; }

}   // namespace

namespace pool {
bool& inside() { static thread_local bool f = false; return f; }
bool dispatch(int count, void (*body)(void*), void* ctx) { return thePool().dispatch(count, body, ctx); }
long contended() { return thePool().contended(); }
}

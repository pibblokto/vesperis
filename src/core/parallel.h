// Row-band parallelism for the per-pixel passes (M10-08) and the band-parallel drawing (B-310). Since B-310 the workers are a
// persistent pool: a frame dispatches a dozen parallel regions (sky, terrain, flora, the mush passes, the RGB conversion), and
// creating threads for each cost more than a millisecond per frame. F-06: the pool deals its work. A dispatch cuts its range into
// chunks that the caller and the workers take from a counter until none is left, so a performance core takes more of them than an
// efficiency core and a band of heavy rows no longer holds up the frame, and the caller starts on them at once (the workers join as
// they wake). The pictures are the serial loop's whoever draws a row: every pass is row-pure (a row's result does not depend on which
// thread makes it or where its chunk begins) or band-pinned (`parallelBands`).
#pragma once
#include <thread>
#include <vector>
#include <algorithm>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <cstdlib>

// the cap on the threads (the cores and `VESPERIS_THREADS` above it are clamped to it). F-06 measured ten on the M4's ten cores: space 4x
// 9.29 -> 8.76 ms, the telescope 4x 22.05 -> 21.11, the surface at 1x-3x 0.03-0.06 slower, and a recorded frame at 4x 7.7 -> 14.5 (the
// PNG writer's thread starved by a pool on every core, as B-422's prefetch was): eight, two cores for the game's own threads
constexpr int POOL_MAX_THREADS = 8;

inline int& parallelThreadsOverride() { static int n = 0; return n; }   // A-02 (tests): over 0 it is the count (1 the serial loop, in one run)
inline int parallelThreads() {
    if (parallelThreadsOverride() > 0) return std::min(POOL_MAX_THREADS, parallelThreadsOverride());
    static int n = 0;
    if (!n) {
        n = (int)std::thread::hardware_concurrency();
        if (const char* e = std::getenv("VESPERIS_THREADS")) n = std::atoi(e);   // B-310: for the bench (VESPERIS_THREADS=1 is the serial baseline)
        if (n < 1) n = 1; if (n > POOL_MAX_THREADS) n = POOL_MAX_THREADS;
    }
    return n;
}

namespace pool {
// F-06: true on a thread inside a dispatch (its caller, every worker): a dispatch from there runs alone on that thread
bool& inside();
// body(ctx) on the caller and on up to count - 1 workers, each call taking its pieces of the work from the body's own counter until
// none is left. Returns once the caller's call has returned and every worker that entered has left: a worker that wakes after the
// work is gone does not enter, so a descheduled one never holds up the join. False, with nothing run, while another thread's
// dispatch holds the pool
bool dispatch(int count, void (*body)(void*), void* ctx);
template <class B> bool run(int count, B& body) { return dispatch(count, [](void* p) { (*static_cast<B*>(p))(); }, &body); }
// the dispatches that found the pool held by another thread and ran alone (the unit wants none: only the main thread dispatches)
long contended();
}

// Calls fn(begin, end) over [0, n) in chunks dealt to the threads (F-06); in one call when the work is small (under two threads'
// `minPerThread`), with one thread, and inside a dispatch. A chunk is about a quarter of a thread's share and at least half of
// `minPerThread`; `chunk` over 0 sets it (the plate's blocks, the probe's columns, the forms' finer chunks).
template <class F>
void parallelFor(int n, int minPerThread, const F& fn, int chunk = 0) {
    const int threads = std::min(parallelThreads(), std::max(1, n / std::max(1, minPerThread)));
    if (threads <= 1 || pool::inside()) { fn(0, n); return; }
    if (chunk <= 0) chunk = std::max(std::max(1, minPerThread / 2), n / (threads * 4));
    std::atomic<int> next{0};
    auto body = [&] {
        const int N = n, C = chunk;
        for (int b; (b = next.fetch_add(C, std::memory_order_relaxed)) < N;) fn(b, std::min(N, b + C));
    };
    if (!pool::run(threads, body)) fn(0, n);
}

// B-423: fn(begin, end) over exactly `bands` contiguous bands of [0, n), whatever the threads: for a pass whose picture depends on
// where its bands begin (the flora's coverage grids are per band). The bands are dealt to the threads (F-06); with one thread (or
// inside a dispatch) they run one after another, so the serial loop draws the bands the workers draw.
template <class F>
void parallelBands(int n, int bands, const F& fn) {
    bands = std::max(1, std::min(bands, n));
    const int band = (n + bands - 1) / bands, threads = std::min(parallelThreads(), bands);
    auto run = [&](int k) { int b = k * band, e = std::min(n, b + band); if (b < e) fn(b, e); };
    if (threads > 1 && !pool::inside()) {
        std::atomic<int> next{0};
        auto body = [&] { for (int k; (k = next.fetch_add(1, std::memory_order_relaxed)) < bands;) run(k); };
        if (pool::run(threads, body)) return;
    }
    for (int k = 0; k < bands; k++) run(k);
}

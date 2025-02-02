// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#if !defined(__STDROMANO_THREADING)
#define __STDROMANO_THREADING

#include "stdromano/stdromano.h"

#include "concurrentqueue.h"

#include <mutex>
#include <atomic>

#if defined(STDROMANO_WIN)
#include "Windows.h"
#elif defined(STDROMANO_LINUX)
#include <unistd.h>
#include <syscall.h>
#endif /* defined(STDROMANO_WIN) */

STDROMANO_NAMESPACE_BEGIN

STDROMANO_FORCE_INLINE size_t get_num_procs() noexcept
{
#if defined(STDROMANO_WIN)
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return static_cast<size_t>(sys_info.dwNumberOfProcessors);
#elif defined(STDROMANO_LINUX)
    return static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN));
#endif /* defined(STDROMANO_WIN) */
}

#if defined(STDROMANO_WIN)
using thread_handle = HANDLE;
using thread_func = void(*)(void*);
#elif defined(STDROMANO_LINUX)
using thread_handle = pthread_t;
using thread_func = void*(*)(void*);
#endif /* defined(STDROMANO_WIN) */

class Mutex 
{
    std::atomic<bool> _flag;

public:
    Mutex() 
    {
        this->_flag.store(false);
    }

    STDROMANO_FORCE_INLINE void lock() noexcept
    {
        while (this->_flag.exchange(true, std::memory_order_relaxed));
        std::atomic_thread_fence(std::memory_order_acquire);
    }

    STDROMANO_FORCE_INLINE void unlock() noexcept
    {
        std::atomic_thread_fence(std::memory_order_release);
        this->_flag.store(false, std::memory_order_relaxed);
    }
};

class ScopedLock
{

};

class STDROMANO_API Thread
{
    thread_handle _handle;
    thread_func _func;
    void* _args = nullptr;
    
#if defined(STDROMANO_WIN)
    DWORD _id;
#elif defined(STDROMANO_LINUX)
    int _id;
#endif /* defined(STDROMANO_WIN) */

    bool _running = false;

public:
    Thread(thread_func func,
           void* args,
           bool daemon = false,
           bool detached = false);

    ~Thread();

    void start() noexcept;

    void detach() noexcept;

    void join() noexcept;

    STDROMANO_FORCE_INLINE bool joined() noexcept
    {
#if defined(STDROMANO_WIN)
        return this->_handle != nullptr;
#elif defined(STDROMANO_LINUX)
        return this->_handle == 0;
#endif /* defined(STDROMANO_WIN) */
    }
};

STDROMANO_FORCE_INLINE size_t thread_get_id() noexcept
{
#if defined(STDROMANO_WIN)
    return static_cast<size_t>(GetCurrentThreadId());
#elif defined(STDROMANO_LINUX)
    return static_cast<size_t>(syscall(SYS_gettid));
#endif /* defined(STDROMANO_WIN) */
}

STDROMANO_FORCE_INLINE void thread_sleep(const size_t sleep_duration_ms) noexcept
{
#if defined(STDROMANO_WIN)
    Sleep(static_cast<DWORD>(sleep_duration_ms));
#elif defined(STDROMANO_LINUX)
    struct timespec wait_duration;
    wait_duration.tv_sec = sleep_duration_ms / 1000;
    wait_duration.tv_nsec = (sleep_duration_ms % 1000) * 1000000;

    nanosleep(&wait_duration, NULL);
#endif /* defined(STDROMANO_WIN) */
}

class STDROMANO_API ThreadPoolWork
{
public:
    ThreadPoolWork() {}

    virtual ~ThreadPoolWork() {}

    virtual void execute() = 0;
};

class STDROMANO_API ThreadPool
{
public:
    ThreadPool(const int64_t workers_count = -1);

    ~ThreadPool();

    bool add_work(ThreadPoolWork* work) noexcept;

    void wait() const noexcept;

    bool is_started() const noexcept { return this->_started.load(); }

    bool is_stopped() const noexcept { return this->_stop.load(); }

#if defined(STDROMANO_WIN)
    static void worker_func(void* args) noexcept;
#elif defined(STDROMANO_LINUX)
    static void* worker_func(void* args) noexcept;
#endif /* defined(STDROMANO_WIN) */

private:
    moodycamel::ConcurrentQueue<ThreadPoolWork*> _work_queue;
    Thread* _workers;

    std::atomic<size_t> _num_workers;
    std::atomic<size_t> _num_active_workers;

    std::atomic<bool> _stop;

    std::atomic<bool> _started;

    void init(const int64_t workers_count) noexcept;
};

class STDROMANO_API GlobalThreadPool 
{
public:
    static GlobalThreadPool& get_instance() noexcept { static GlobalThreadPool tp; return tp; }

    GlobalThreadPool(const GlobalThreadPool&) = delete;
    GlobalThreadPool& operator=(const GlobalThreadPool&) = delete;

    bool add_work(ThreadPoolWork* work) noexcept 
    { 
        return this->_tp->add_work(std::forward<ThreadPoolWork*>(work));
    }

    void wait() const noexcept { return this->_tp->wait(); }

    void stop() noexcept { this->_stopped = true; this->~GlobalThreadPool(); }

private:
    GlobalThreadPool();
    ~GlobalThreadPool();

    ThreadPool* _tp = nullptr;

    bool _started = false;
    bool _stopped = false;
};

/* Macro to make the code more undestandable and readable */
#define global_threadpool GlobalThreadPool::get_instance()

STDROMANO_API void atexit_handler_global_threadpool() noexcept;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_THREADING) */
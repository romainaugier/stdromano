// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#if !defined(__STDROMANO_THREADING)
#define __STDROMANO_THREADING

#include "stdromano/stdromano.hpp"

#include "concurrentqueue.h"

#include <atomic>
#include <functional>

#if defined(STDROMANO_WIN)
#include "Windows.h"
#elif defined(STDROMANO_LINUX)
#include <syscall.h>
#include <unistd.h>

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
using thread_func = void (*)(void*);
#elif defined(STDROMANO_LINUX)
using thread_handle = pthread_t;
using thread_func = void* (*)(void*);
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
        while(this->_flag.exchange(true, std::memory_order_relaxed))
            ;
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
private:
    thread_handle _handle;
    std::function<void()> _task;

#if defined(STDROMANO_WIN)
    DWORD _id;

    static DWORD WINAPI ThreadProc(LPVOID param)
    {
        Thread* thread = static_cast<Thread*>(param);
        thread->_task();
        return 0;
    }
#elif defined(STDROMANO_LINUX)
    int _id;

    static void* ThreadProc(void* param)
    {
        Thread* thread = static_cast<Thread*>(param);
        thread->_task();
        return nullptr;
    }
#endif /* defined(STDROMANO_WIN) */

    bool _running = false;
    bool _daemon = false;

public:
    Thread(std::function<void()> func, bool daemon = false, bool detached = false)
    {
        this->_task = std::move(func);

#if defined(STDROMANO_WIN)
        this->_handle = CreateThread(NULL, 0, ThreadProc, this, CREATE_SUSPENDED, &this->_id);
#elif defined(STDROMANO_LINUX)
        this->_handle = 0;
#endif /* defined(STDROMANO_WIN) */

        this->_daemon = daemon;
        this->_running = false;

        if(detached)
        {
            this->detach();
        }
    }

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
    ThreadPoolWork()
    {
    }

    virtual ~ThreadPoolWork()
    {
    }

    virtual void execute() = 0;
};

class STDROMANO_API ThreadPool
{
    class STDROMANO_API LambdaWork : public ThreadPoolWork
    {
        std::function<void()> _func;

    public:
        explicit LambdaWork(std::function<void()> func)
            : _func(std::move(func))
        {
        }

        ~LambdaWork() override = default;

        void execute() override
        {
            if(this->_func != nullptr)
            {
                this->_func();
            }
        }
    };

public:
    ThreadPool(const int64_t workers_count = -1);

    ~ThreadPool();

    bool add_work(ThreadPoolWork* work) noexcept;

    STDROMANO_FORCE_INLINE bool add_work(std::function<void()>&& func) noexcept
    {
        LambdaWork* work = new LambdaWork(std::forward<std::function<void()>&&>(func));
        return this->add_work(work);
    }

    void wait() const noexcept;

    STDROMANO_FORCE_INLINE bool is_started() const noexcept
    {
        return this->_started.load();
    }

    STDROMANO_FORCE_INLINE bool is_stopped() const noexcept
    {
        return this->_stop.load();
    }

    STDROMANO_FORCE_INLINE std::size_t num_workers() const noexcept
    {
        return this->_num_workers.load();
    }

    STDROMANO_FORCE_INLINE void set_max_active_workers(std::size_t max_workers) noexcept
    {
        this->_max_active_workers.store(max_workers);
    }

private:
    moodycamel::ConcurrentQueue<ThreadPoolWork*> _work_queue;
    Thread* _workers;

    std::atomic<std::size_t> _num_workers;
    std::atomic<std::size_t> _num_active_workers;

    std::atomic<std::size_t> _max_active_workers;

    std::atomic<bool> _stop;

    std::atomic<bool> _started;

    void init(const int64_t workers_count) noexcept;
};

class STDROMANO_API GlobalThreadPool
{
public:
    static GlobalThreadPool& get_instance() noexcept;

    GlobalThreadPool(const GlobalThreadPool&) = delete;
    GlobalThreadPool& operator=(const GlobalThreadPool&) = delete;

    bool add_work(ThreadPoolWork* work) noexcept
    {
        return this->_tp->add_work(std::forward<ThreadPoolWork*>(work));
    }

    bool add_work(std::function<void()>&& work) noexcept
    {
        return this->_tp->add_work(std::forward<std::function<void()>>(work));
    }

    void wait() const noexcept
    {
        return this->_tp->wait();
    }

    void stop() noexcept
    {
        this->~GlobalThreadPool();
    }

    STDROMANO_FORCE_INLINE std::size_t num_workers() const noexcept
    {
        return this->_tp->num_workers();
    }

    STDROMANO_FORCE_INLINE void set_max_active_workers(std::size_t max_active_workers) noexcept
    {
        this->_tp->set_max_active_workers(max_active_workers);
    }

private:
    GlobalThreadPool();
    ~GlobalThreadPool();

    ThreadPool* _tp = nullptr;
};

/* Macro to make the code more understandable and readable */
#define global_threadpool GlobalThreadPool::get_instance()

STDROMANO_API void atexit_handler_global_threadpool() noexcept;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_THREADING) */
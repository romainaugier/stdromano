// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#if !defined(__STDROMANO_THREADING)
#define __STDROMANO_THREADING

#include "stdromano/stdromano.hpp"
#include "stdromano/atomic.hpp"
#include "stdromano/memory.hpp"

#include "concurrentqueue.h"

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

class Thread
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

    ~Thread()
    {
        if(this->_running)
        {
            this->join();
        }
    }

    void start() noexcept
    {
    #if defined(STDROMANO_WIN)
        ResumeThread(this->_handle);
    #elif defined(STDROMANO_LINUX)
        pthread_create(&this->_handle, NULL, ThreadProc, this);
    #endif /* defined(STDROMANO_WIN) */

        this->_running = true;
    }

    void detach() noexcept
    {
    #if defined(STDROMANO_WIN)
        if(_handle)
        {
            CloseHandle(this->_handle);
            this->_handle = nullptr;
        }
    #elif defined(STDROMANO_LINUX)
        if(this->_handle)
        {
            pthread_detach(this->_handle);
            this->_handle = 0;
        }
    #endif /* defined(STDROMANO_WIN) */
    }

    void join() noexcept
    {
        if(!this->_running)
        {
            return;
        }

    #if defined(STDROMANO_WIN)
        WaitForSingleObject(this->_handle, INFINITE);
        CloseHandle(this->_handle);
        this->_handle = nullptr;
    #elif defined(STDROMANO_LINUX)
        pthread_join(this->_handle, NULL);
        this->_handle = 0;
    #endif /* defined(STDROMANO_WIN) */

        this->_running = false;
    }

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

STDROMANO_FORCE_INLINE void thread_yield() noexcept
{
    std::this_thread::yield();
}

/********************************/
/* ThreadPool */
/********************************/

class ThreadPool;
class ThreadPoolWork;

class ThreadPoolWaiter
{
    friend class ThreadPool;
    friend class StealingThreadPool;
    friend class ThreadPoolWork;

    Atomic<std::size_t> _expected{0};
    Atomic<std::size_t> _done{0};

public:
    ThreadPoolWaiter() {}

    void wait() noexcept
    {
        while(this->_done.load() != this->_expected.load())
        {
            thread_yield();
        }
    }
};

class ThreadPoolWork
{
    friend class ThreadPool;
    friend class StealingThreadPool;

    ThreadPoolWaiter* _waiter = nullptr;

public:
    ThreadPoolWork() {}

    virtual ~ThreadPoolWork() noexcept
    {
        if(this->_waiter != nullptr)
        {
            ++this->_waiter->_done;
        }
    }

    virtual void execute() = 0;
};

class ThreadPool
{
    class LambdaWork : public ThreadPoolWork
    {
        std::function<void()> _func;

    public:
        explicit LambdaWork(std::function<void()> func) : _func(std::move(func)) {}

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
    explicit ThreadPool(const int64_t workers_count = -1)
    {
        this->_workers = nullptr;

        this->init(workers_count);
    }

    ~ThreadPool()
    {
        if(this->_workers != nullptr)
        {
            const size_t num_workers = this->_num_workers.load();

            this->_stop.store(true);

            for(size_t i = 0; i < num_workers; i++)
            {
                this->_workers[i].join();
            }

            mem_free(this->_workers);

            this->_workers = nullptr;
        }
    }

    bool add_work(ThreadPoolWork* work, ThreadPoolWaiter* waiter = nullptr) noexcept
    {
        work->_waiter = waiter;

        if(!this->_work_queue.enqueue(work))
        {
            return false;
        }

        if(waiter != nullptr)
        {
            ++waiter->_expected;
        }

        return true;
    }

    STDROMANO_FORCE_INLINE bool add_work(std::function<void()>&& func,
                                         ThreadPoolWaiter* waiter = nullptr) noexcept
    {
        LambdaWork* work = new LambdaWork(std::forward<std::function<void()>&&>(func));
        return this->add_work(work, waiter);
    }

    STDROMANO_FORCE_INLINE void wait() noexcept
    {
        while(this->_work_queue.size_approx() > 0 || this->_num_active_workers.load() > 0)
        {
            thread_yield();
        }
    }

    STDROMANO_FORCE_INLINE bool is_started() const noexcept
    {
        return this->_started.load();
    }

    STDROMANO_FORCE_INLINE bool is_stopped() const noexcept
    {
        return this->_stop.load();
    }

    STDROMANO_FORCE_INLINE std::size_t num_workers() noexcept
    {
        return this->_num_workers.load();
    }

    STDROMANO_FORCE_INLINE void set_max_active_workers(std::size_t max_workers) noexcept
    {
        this->_max_active_workers.store(max_workers);
    }

    STDROMANO_API static ThreadPool& get_global_threadpool() noexcept;

private:
    moodycamel::ConcurrentQueue<ThreadPoolWork*> _work_queue;

    Thread* _workers;

    Atomic<std::size_t> _num_workers{0};
    Atomic<std::size_t> _num_active_workers{0};

    Atomic<std::size_t> _max_active_workers{0};

    Atomic<bool> _stop{false};

    Atomic<bool> _started{false};

    void init(const int64_t workers_count) noexcept
    {
        const size_t num_workers = workers_count < 1 ? get_num_procs() :
                                                       static_cast<size_t>(workers_count);

        this->_workers = mem_alloc<Thread>(num_workers * sizeof(Thread));

        for(size_t i = 0; i < num_workers; i++)
        {
            ::new(std::addressof(this->_workers[i])) Thread(
                [&]()
                {
                    ThreadPool* tp = this;

                    while(!tp->_stop.load())
                    {
                        if(tp->_num_active_workers.load() >= tp->_max_active_workers.load())
                        {
                            continue;
                        }

                        if(tp->_work_queue.size_approx() > 0)
                        {
                            ThreadPoolWork* work = nullptr;

                            if(tp->_work_queue.try_dequeue(work))
                            {
                                tp->_num_active_workers++;

                                try
                                {
                                    if(work != nullptr)
                                    {
                                        work->execute();

                                        delete work;
                                    }
                                }
                                catch(const std::exception& e)
                                {
                                    std::fprintf(stderr,
                                                "Error caught while executing threadpool work %s",
                                                e.what());
                                }

                                tp->_num_active_workers--;
                            }
                        }
                    }
                });

            this->_workers[i].start();
            this->_num_workers++;
            this->_max_active_workers++;
        }

        this->_started.store(true);
    }
};

class StealingThreadPool
{
private:
    class LambdaWork : public ThreadPoolWork
    {
        std::function<void()> _func;
    public:
        explicit LambdaWork(std::function<void()> func) : _func(std::move(func)) {}
        ~LambdaWork() override = default;

        void execute() override
        {
            if(this->_func != nullptr)
            {
                this->_func();
            }
        }
    };

    struct WorkerContext
    {
        moodycamel::ConcurrentQueue<ThreadPoolWork*> local_queue;
        Atomic<std::size_t> queue_size{0};
        std::unique_ptr<Thread> thread;
        Atomic<bool> active{true};
    };

    std::vector<std::unique_ptr<WorkerContext>> _workers;
    moodycamel::ConcurrentQueue<ThreadPoolWork*> _global_queue;
    Atomic<std::size_t> _global_queue_size{0};

    Atomic<bool> _running{false};
    Atomic<bool> _shutdown{false};
    Atomic<std::size_t> _active_workers{0};
    Atomic<std::size_t> _max_active_workers{0};
    Atomic<std::size_t> _total_pending_work{0};

    std::size_t _num_workers;
    Atomic<std::size_t> _next_worker{0};

    void worker_loop(std::size_t worker_id) noexcept
    {
        WorkerContext* ctx = this->_workers[worker_id].get();
        ThreadPoolWork* work = nullptr;

        const std::size_t num_steal_attempts = this->_num_workers * 2;

        while(!this->_shutdown.load(MemoryOrder::Acquire))
        {
            bool found_work = false;

            if(ctx->local_queue.try_dequeue(work))
            {
                ctx->queue_size.fetch_sub(1, MemoryOrder::Release);
                this->_total_pending_work.fetch_sub(1, MemoryOrder::Release);
                found_work = true;
            }
            else if(this->_global_queue.try_dequeue(work))
            {
                this->_global_queue_size.fetch_sub(1, MemoryOrder::Release);
                this->_total_pending_work.fetch_sub(1, MemoryOrder::Release);
                found_work = true;
            }
            else
            {
                for(std::size_t i = 0; i < num_steal_attempts && !found_work; ++i)
                {
                    std::size_t victim = (worker_id + i + 1) % this->_num_workers;

                    if(victim != worker_id && this->_workers[victim]->queue_size.load(MemoryOrder::Acquire) > 0)
                    {
                        if(this->_workers[victim]->local_queue.try_dequeue(work))
                        {
                            this->_workers[victim]->queue_size.fetch_sub(1, MemoryOrder::Release);
                            this->_total_pending_work.fetch_sub(1, MemoryOrder::Release);
                            found_work = true;
                            break;
                        }
                    }
                }
            }

            if(found_work && work != nullptr)
            {
                std::size_t max_workers = this->_max_active_workers.load(MemoryOrder::Acquire);

                if(max_workers > 0)
                {
                    std::size_t current_active = this->_active_workers.load(MemoryOrder::Acquire);

                    if(current_active >= max_workers)
                    {
                        ctx->local_queue.enqueue(work);
                        ctx->queue_size.fetch_add(1, MemoryOrder::Release);
                        this->_total_pending_work.fetch_add(1, MemoryOrder::Release);
                        thread_yield();
                        continue;
                    }
                }

                this->_active_workers.fetch_add(1, MemoryOrder::Acquire);

                try
                {
                    work->execute();
                }
                catch(...)
                {
                }

                delete work;

                this->_active_workers.fetch_sub(1, MemoryOrder::Release);
            }
            else
            {
                thread_yield();
            }
        }
    }

public:
    explicit StealingThreadPool(const int64_t workers_count = -1)
    {
        this->_num_workers = workers_count <= 0 ? get_num_procs() :
                                                  static_cast<std::size_t>(workers_count);

        this->_workers.reserve(this->_num_workers);

        for(std::size_t i = 0; i < this->_num_workers; ++i)
        {
            auto ctx = std::make_unique<WorkerContext>();

            ctx->thread = std::make_unique<Thread>(
                [this, i]() { this->worker_loop(i); },
                false,
                false
            );

            this->_workers.push_back(std::move(ctx));
        }

        this->_running.store(true, MemoryOrder::Release);

        for(auto& worker : this->_workers)
        {
            worker->thread->start();
        }
    }

    ~StealingThreadPool()
    {
        this->_shutdown.store(true, MemoryOrder::Release);

        for(auto& worker : this->_workers)
            if(worker->thread)
                worker->thread->join();

        ThreadPoolWork* work = nullptr;

        while(this->_global_queue.try_dequeue(work))
            delete work;

        for(auto& worker : this->_workers)
            while(worker->local_queue.try_dequeue(work))
                delete work;

        this->_running.store(false, MemoryOrder::Release);
    }

    bool add_work(ThreadPoolWork* work, ThreadPoolWaiter* waiter = nullptr) noexcept
    {
        if(!this->_running.load(MemoryOrder::Acquire) || work == nullptr)
            return false;

        work->_waiter = waiter;

        if(waiter != nullptr)
            waiter->_expected.fetch_add(1, MemoryOrder::Release);

        std::size_t target_worker = _next_worker.fetch_add(1, MemoryOrder::Relaxed) % _num_workers;

        this->_workers[target_worker]->local_queue.enqueue(work);
        this->_workers[target_worker]->queue_size.fetch_add(1, MemoryOrder::Release);
        this->_total_pending_work.fetch_add(1, MemoryOrder::Release);

        return true;
    }

    STDROMANO_FORCE_INLINE bool add_work(std::function<void()>&& func,
                                         ThreadPoolWaiter* waiter = nullptr) noexcept
    {
        if(!this->_running.load(MemoryOrder::Acquire))
            return false;

        LambdaWork* work = new (std::nothrow) LambdaWork(std::move(func));

        if(work == nullptr)
            return false;

        return add_work(work, waiter);
    }

    STDROMANO_FORCE_INLINE void wait() noexcept
    {
        while(this->_total_pending_work.load(MemoryOrder::Acquire) > 0 ||
              this->_active_workers.load(MemoryOrder::Acquire) > 0)
            thread_yield();
    }

    STDROMANO_FORCE_INLINE bool is_started() const noexcept
    {
        return this->_running.load(MemoryOrder::Acquire);
    }

    STDROMANO_FORCE_INLINE bool is_stopped() const noexcept
    {
        return !this->_running.load(MemoryOrder::Acquire);
    }

    STDROMANO_FORCE_INLINE std::size_t num_workers() noexcept
    {
        return this->_num_workers;
    }

    STDROMANO_FORCE_INLINE void set_max_active_workers(std::size_t max_workers) noexcept
    {
        this->_max_active_workers.store(max_workers, MemoryOrder::Release);
    }

    STDROMANO_API static StealingThreadPool& get_global_threadpool() noexcept;
};

/* Macro to make the code more understandable and readable */
#define global_threadpool() StealingThreadPool::get_global_threadpool()

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_THREADING) */

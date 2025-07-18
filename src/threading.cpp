// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/threading.hpp"
#include "stdromano/memory.hpp"

#include <cstdio>

STDROMANO_NAMESPACE_BEGIN

Thread::~Thread()
{
    if(this->_running)
    {
        this->join();
    }
}

void Thread::start() noexcept
{
#if defined(STDROMANO_WIN)
    ResumeThread(this->_handle);
#elif defined(STDROMANO_LINUX)
    pthread_create(&this->_handle, NULL, ThreadProc, this);
#endif /* defined(STDROMANO_WIN) */

    this->_running = true;
}

void Thread::detach() noexcept
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

void Thread::join() noexcept
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

ThreadPool::ThreadPool(const int64_t workers_count)
{
    this->_num_workers.store(0);
    this->_num_active_workers.store(0);

    this->_stop.store(false);
    this->_started.store(false);

    this->_workers = nullptr;

    this->init(workers_count);
}

ThreadPool::~ThreadPool()
{
    if(this->_workers != nullptr)
    {
        const size_t num_workers = this->_num_workers.load();

        this->_stop = true;

        while(this->_num_workers.load() > 0)
        {
            thread_sleep(1);
        }

        for(size_t i = 0; i < num_workers; i++)
        {
            this->_workers[i].join();
            this->_workers[i].~Thread();
        }

        mem_free(this->_workers);
        this->_workers = nullptr;
    }
}

void ThreadPool::init(const int64_t workers_count) noexcept
{
    const size_t num_workers =
        workers_count < 1 ? get_num_procs() : static_cast<size_t>(workers_count);

    this->_workers = static_cast<Thread*>(mem_alloc(num_workers * sizeof(Thread)));

    for(size_t i = 0; i < num_workers; i++)
    {
        ::new(std::addressof(this->_workers[i])) Thread(
            [&]()
            {
                ThreadPool* tp = this;

                while(true)
                {
                    const bool stop = tp->_stop.load();

                    if(stop)
                    {
                        break;
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

                tp->_num_workers--;
            });

        this->_workers[i].start();
        this->_num_workers++;
    }

    this->_started.store(true);
}

bool ThreadPool::add_work(ThreadPoolWork* work) noexcept
{
    return this->_work_queue.enqueue(work);
}

void ThreadPool::wait() const noexcept
{
    while(this->_work_queue.size_approx() > 0 || this->_num_active_workers.load() > 0)
    {
        thread_sleep(1);
    }
}

static bool g_global_threadpool_started = false;

GlobalThreadPool::GlobalThreadPool()
{
    if(this->_tp == nullptr)
    {
        this->_tp = new ThreadPool();
        g_global_threadpool_started = true;
    }
}

GlobalThreadPool::~GlobalThreadPool()
{
    if(this->_tp != nullptr)
    {
        delete this->_tp;
        this->_tp = nullptr;
    }
}

void atexit_handler_global_threadpool() noexcept
{
    if(g_global_threadpool_started)
    {
        global_threadpool.stop();
    }
}

STDROMANO_NAMESPACE_END
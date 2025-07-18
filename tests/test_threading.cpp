// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/threading.hpp"

#include <cstdio>

#if defined(STDROMANO_WIN)
void t1_func()
#elif defined(STDROMANO_LINUX)
void* t1_func()
#endif /* defined(STDROMANO_WIN) */
{
    stdromano::thread_sleep(500);

    std::printf("Hello from t1\n");

#if defined(STDROMANO_LINUX)
    return nullptr;
#endif /* defined(STDROMANO_LINUX) */
}

#if defined(STDROMANO_WIN)
void t2_func()
#elif defined(STDROMANO_LINUX)
void* t2_func()
#endif /* defined(STDROMANO_WIN) */
{
    std::printf("Hello from t2\n");

#if defined(STDROMANO_LINUX)
    return nullptr;
#endif /* defined(STDROMANO_LINUX) */
}

class TPoolWork : public stdromano::ThreadPoolWork
{
  public:
    TPoolWork(const size_t job_id)
    {
        this->_job_id = job_id;
    }

    virtual ~TPoolWork() override
    {
    }

    virtual void execute() override
    {
        std::printf("Hello with job: %zu (tid: %zu)\n", this->_job_id, stdromano::thread_get_id());
    }

  private:
    size_t _job_id;
};

void atexit_handler_stdromano_global_threadpool()
{
    stdromano::atexit_handler_global_threadpool();
}

#define NUM_WORKS 100

int main()
{
    STDROMANO_ATEXIT_REGISTER(atexit_handler_stdromano_global_threadpool, true);

    std::printf("Starting threading test\n");

    stdromano::Thread t1(t1_func);
    stdromano::Thread t2(t2_func);
    stdromano::Thread t3(
                   []() {
                       std::printf("Hello from t3 lambda: (tid: %zu)\n",
                                   stdromano::thread_get_id());
                   });

    t1.start();
    t2.start();
    t3.start();

    stdromano::thread_sleep(1000);

    t1.join();
    t2.join();
    t3.join();

    for(size_t i = 0; i < NUM_WORKS; i++)
    {
        TPoolWork* work = new TPoolWork(i);

        stdromano::global_threadpool.add_work(work);
    }

    for(size_t i = 0; i < NUM_WORKS; i++)
    {
        stdromano::global_threadpool.add_work([&]() { std::printf("Hello from lambda work\n"); });
    }

    std::printf("Finished adding work\n");

    stdromano::global_threadpool.wait();

    std::printf("Finished threading test\n");

    return 0;
}
// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#include "stdromano/threading.h"

#include <cstdio>

void* t1_func(void* args)
{
    stdromano::thread_sleep(500);

    std::printf("Hello from t1\n");

    return nullptr;
}

void* t2_func(void* args)
{
    std::printf("Hello from t2\n");

    return nullptr;
}

class TPoolWork : public stdromano::ThreadPoolWork
{
public:
    TPoolWork(const size_t job_id)
    {
        this->_job_id = job_id;
    }

    virtual ~TPoolWork() override {}

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

#define NUM_WORKS 1000

int main()
{
    STDROMANO_ATEXIT_REGISTER(atexit_handler_stdromano_global_threadpool, true);

    std::printf("Starting threading test\n");

    stdromano::Thread t1(&t1_func, nullptr);
    stdromano::Thread t2(&t2_func, nullptr);

    t1.start();
    t2.start();

    stdromano::thread_sleep(1000);

    t1.join();
    t2.join();

    for(size_t i = 0; i < NUM_WORKS; i++)
    {
        TPoolWork* work = new TPoolWork(i);

        stdromano::global_threadpool.add_work(work);
    }

    std::printf("Finished adding work\n");

    stdromano::global_threadpool.wait();

    std::printf("Finished threading test\n");

    return 0;
}
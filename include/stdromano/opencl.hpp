// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#include "stdromano/stdromano.hpp"
#if !defined(__STDROMANO_OPENCL)
#define __STDROMANO_OPENCL

#include "stdromano/hashmap.hpp"
#include "stdromano/string.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/atomic.hpp"

#include <CL/opencl.hpp>

STDROMANO_NAMESPACE_BEGIN

struct OpenCLConfig
{
    bool enable_profiling = true;
    bool prefer_dedicated_gpu = true;
    StringD preferred_vendor;
    size_t max_cache_size = 100;
    bool enable_debug_output = false;
    cl_device_type device_type = CL_DEVICE_TYPE_GPU;
};

class STDROMANO_API TaskGroup
{
private:
    std::vector<cl::Event> _events;
    mutable std::mutex _events_mutex;

public:
    void add_event(cl::Event event) noexcept
    {
        std::lock_guard<std::mutex> lock(this->_events_mutex);
        this->_events.push_back(event);
    }

    void wait_all() noexcept
    {
        std::lock_guard<std::mutex> lock(this->_events_mutex);

        if(!this->_events.empty())
        {
            cl::Event::waitForEvents(this->_events);
        }
    }

    void clear() noexcept
    {
        std::lock_guard<std::mutex> lock(this->_events_mutex);
        this->_events.clear();
    }

    size_t size() const noexcept
    {
        std::lock_guard<std::mutex> lock(this->_events_mutex);
        return this->_events.size();
    }
};

struct DeviceInfo
{
    cl::Device device;
    StringD name;
    StringD vendor;
    cl_device_type type;
    size_t global_mem_size;
    size_t local_mem_size;
    cl_uint compute_units;
    size_t max_work_group_size;
    bool is_dedicated;

    DeviceInfo() = default;

    DeviceInfo(const cl::Device& dev)
        : device(dev)
    {
        name = dev.getInfo<CL_DEVICE_NAME>().c_str();
        vendor = dev.getInfo<CL_DEVICE_VENDOR>().c_str();
        type = dev.getInfo<CL_DEVICE_TYPE>();
        global_mem_size = dev.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
        local_mem_size = dev.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
        compute_units = dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
        max_work_group_size = dev.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();

        is_dedicated = (type == CL_DEVICE_TYPE_GPU) && (global_mem_size > 1024 * 1024 * 1024);
    }
};

class STDROMANO_API OpenCLManager
{
public:
    using ProgramCacheKey = std::tuple<StringD, StringD, cl_device_id>;

    struct ProgramCacheKeyHash
    {
        std::size_t operator()(const ProgramCacheKey& key) const
        {
            auto h1 = std::hash<StringD>{}(std::get<0>(key));
            auto h2 = std::hash<StringD>{}(std::get<1>(key));
            auto h3 = std::hash<cl_device_id>{}(std::get<2>(key));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    static OpenCLManager& get_instance() noexcept;

    OpenCLManager(const OpenCLManager&) = delete;
    OpenCLManager& operator=(const OpenCLManager&) = delete;

    STDROMANO_NO_DISCARD bool initialize(const OpenCLConfig& config = OpenCLConfig{}) noexcept;

    STDROMANO_FORCE_INLINE bool is_initialized() const noexcept
    {
        return this->_initialized.load();
    }

    /* Returns nullptr if not initialized */
    STDROMANO_FORCE_INLINE cl::Context* get_context() noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(!this->is_initialized())
        {
            return nullptr;
        }

        return std::addressof(this->_context);
    }

    /* Returns nullptr if not initialized */
    STDROMANO_FORCE_INLINE const std::vector<DeviceInfo>* get_device_info() const
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(!this->is_initialized())
        {
            return nullptr;
        }

        return std::addressof(this->_device_info);
    }

    /* Returns nullptr if not initialized */
    STDROMANO_FORCE_INLINE const std::vector<cl::Device>* get_devices() const
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(!this->is_initialized())
        {
            return nullptr;
        }

        return std::addressof(_devices);
    }

    /* Returns nullptr if not initialized or index >= number of queues */
    STDROMANO_FORCE_INLINE cl::CommandQueue* get_queue(std::size_t device_index = 0) noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(!this->is_initialized())
        {
            return nullptr;
        }

        if(device_index >= this->_queues.size())
        {
            return nullptr;
        }

        return std::addressof(this->_queues[device_index]);
    }

    /* Returns nullptr if not initialized or queues is empty */
    STDROMANO_FORCE_INLINE cl::CommandQueue* get_next_queue() noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(!this->is_initialized())
        {
            return nullptr;
        }

        if(this->_queues.empty())
        {
            return nullptr;
        }

        return std::addressof(this->_queues[this->_queue_counter++ % this->_queues.size()]);
    }

    STDROMANO_FORCE_INLINE std::size_t get_num_devices() const noexcept
    {
        return this->_queues.size();
    }

    template <typename T>
    cl::Buffer create_buffer(std::size_t count, cl_mem_flags flags = CL_MEM_READ_WRITE) 
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");
        STDROMANO_ASSERT(count > 0, "Buffer cannot be zero");

        return cl::Buffer(this->_context, flags, sizeof(T) * count);
    }

    template <typename T>
    void write_buffer(const cl::Buffer& buffer,
                      const T* data,
                      const std::size_t size,
                      std::size_t device_index = 0,
                      bool blocking = true) noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(!this->is_initialized())
        {
            return;
        }

        if(device_index >= this->_queues.size())
        {
            return;
        }

        cl_int err = this->_queues[device_index].enqueueWriteBuffer(buffer,
                                                                    blocking ? CL_TRUE : CL_FALSE,
                                                                    0,
                                                                    sizeof(T) * size,
                                                                    data);
        
        if(err != CL_SUCCESS)
        {
            log_error("OpenCL error: failed to write buffer: {}", get_cl_error_string(err));
        }
    }

    bool copy_buffer(const cl::Buffer& to,
                     const cl::Buffer& from,
                     std::size_t size,
                     std::size_t device_index = 0,
                     bool blocking = true) noexcept;

    template <typename T>
    STDROMANO_NO_DISCARD bool read_buffer(const cl::Buffer& buffer,
                                          T* data,
                                          std::size_t count,
                                          std::size_t device_index = 0,
                                          bool blocking = true)
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(device_index >= this->_queues.size())
        {
            return false;
        }

        cl_int err = this->_queues[device_index].enqueueReadBuffer(buffer,
                                                                   blocking ? CL_TRUE : CL_FALSE,
                                                                   0,
                                                                   sizeof(T) * count,
                                                                   data);
        
        if(err != CL_SUCCESS)
        {
            log_error("OpenCL error: failed to read buffer: {}", get_cl_error_string(err));
            return false;
        }

        return true;
    }

    STDROMANO_NO_DISCARD bool build_program_from_source(cl::Program& ret_program,
                                                        const StringD& source,
                                                        const StringD& build_options = "",
                                                        const cl::Device* specific_device = nullptr) noexcept;

    STDROMANO_NO_DISCARD bool schedule_task(cl::Event& ret_event,
                                            const StringD& kernel_source,
                                            const StringD& kernel_name,
                                            std::function<void(cl::Kernel&)> set_kernel_args,
                                            const cl::NDRange& global_size,
                                            const cl::NDRange& local_size = cl::NullRange,
                                            const StringD& build_options = "",
                                            std::size_t device_index = 0,
                                            const std::vector<cl::Event>* wait_events = nullptr) noexcept;

    double get_execution_time_ms(const cl::Event& event) noexcept;

    void finish_all_queues() noexcept;

    STDROMANO_FORCE_INLINE size_t get_cache_size() const noexcept
    {
        std::lock_guard<std::mutex> lock(this->_cache_mutex);
        return this->_program_cache.size();
    }

    STDROMANO_FORCE_INLINE void clear_cache() noexcept
    {
        std::lock_guard<std::mutex> lock(this->_cache_mutex);
        this->_program_cache.clear();
    }

    const StringD& get_kernel_source(const StringD& name) const noexcept;

    bool has_kernel_source(const StringD& name) const noexcept;

private:
    OpenCLManager() = default;

    ~OpenCLManager()
    {
        this->cleanup();
    }

    void cleanup()
    {
        for(auto& queue : this->_queues)
        {
            queue.finish();
        }

        this->_queues.clear();
        this->_devices.clear();
        this->_device_info.clear();
        this->_program_cache.clear();
        this->_initialized.store(false);
    }

    STDROMANO_NO_DISCARD bool setup_platform_and_devices() noexcept;

    STDROMANO_NO_DISCARD bool create_context_and_queues() noexcept;

    void print_device_info() const noexcept;

    StringD get_device_type_string(cl_device_type type) const noexcept;

    StringD get_cl_error_string(cl_int error) const noexcept;

    cl::Platform _selected_platform;
    cl::Context _context;
    std::vector<cl::Device> _devices;
    std::vector<DeviceInfo> _device_info;
    std::vector<cl::CommandQueue> _queues;

    OpenCLConfig _config;

    HashMap<ProgramCacheKey, cl::Program, ProgramCacheKeyHash> _program_cache;
    mutable std::mutex _cache_mutex;

    atomic<size_t> _queue_counter{0};

    atomic<bool> _initialized{false};
    std::mutex _init_mutex;
};

#define opencl_manager OpenCLManager::get_instance()

STDROMANO_NAMESPACE_END

#if defined(STDROMANO_WIN)
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API stdromano::HashMap<stdromano::OpenCLManager::ProgramCacheKey,
                                                                          cl::Program,
                                                                          stdromano::OpenCLManager::ProgramCacheKeyHash>;

STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API std::vector<cl::CommandQueue>;
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API std::vector<stdromano::DeviceInfo>;
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API std::vector<cl::Device>;
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API std::vector<cl::Event>;
#endif /* defined(STDROMANO_WIN) */

#endif /* !defined(__STDROMANO_OPENCL) */

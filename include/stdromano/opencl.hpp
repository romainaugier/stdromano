// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_OPENCL)
#define __STDROMANO_OPENCL

#include "stdromano/stdromano.hpp"
#include "stdromano/hashmap.hpp"
#include "stdromano/string.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/atomic.hpp"
#include "stdromano/filesystem.hpp"

#include <CL/opencl.hpp>

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

STDROMANO_API const char* get_cl_error_string(cl_int error) noexcept;

STDROMANO_API const char* get_cl_device_type_string(cl_device_type type) noexcept;

DETAIL_NAMESPACE_END

struct OpenCLConfig
{
    bool enable_profiling = true;
    bool prefer_dedicated_gpu = true;
    StringD preferred_vendor;
    size_t max_cache_size = 100;
    bool enable_debug_output = false;
    cl_device_type device_type = CL_DEVICE_TYPE_GPU;
};

class TaskGroup
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

class OpenCLManager
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

    STDROMANO_API static OpenCLManager& get_instance() noexcept;

    OpenCLManager(const OpenCLManager&) = delete;
    OpenCLManager& operator=(const OpenCLManager&) = delete;

    STDROMANO_NO_DISCARD bool initialize(const OpenCLConfig& config = OpenCLConfig{}) noexcept
    {
        if(this->is_initialized())
        {
            return true;
        }

        std::lock_guard<std::mutex> lock(this->_init_mutex);

        this->_config = config;

        if(!this->setup_platform_and_devices())
        {
            log_error("OpenCL error: Initialization failed");
            this->cleanup();
            return false;
        }

        if(!create_context_and_queues())
        {
            log_error("OpenCL error: Initialization failed");
            this->cleanup();
            return false;
        }

        this->_initialized.store(true);

        if(this->_config.enable_debug_output)
        {
            this->print_device_info();
        }

        return true;
    }

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

    STDROMANO_NO_DISCARD bool copy_buffer(const cl::Buffer& to,
                                          const cl::Buffer& from,
                                          std::size_t size,
                                          std::size_t device_index = 0,
                                          bool blocking = true) noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(device_index >= this->_queues.size())
        {
            return false;
        }

        cl::Event event;

        cl_int err = this->_queues[device_index].enqueueCopyBuffer(from, to, 0, 0, size, nullptr, &event);

        if(err != CL_SUCCESS)
        {
            log_error("OpenCL error: failed to copy buffer: {}", this->get_cl_error_string(err));
            return false;
        }

        if(blocking)
        {
            event.wait();
        }

        return true;
    }

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
                                                        const cl::Device* specific_device = nullptr) noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        const auto& target_device = specific_device ? *specific_device : _devices[0];
        const auto cache_key = std::make_tuple(source, build_options, target_device());

        {
            std::lock_guard<std::mutex> lock(_cache_mutex);

            if(auto it = this->_program_cache.find(cache_key); it != this->_program_cache.end())
            {
                ret_program = it->second;
                return true;
            }
        }

        ret_program = cl::Program(this->_context,
                                    cl::Program::Sources{{ source.c_str(),
                                                        source.length() }});

        std::vector<cl::Device> build_devices;

        if(specific_device)
        {
            build_devices = { *specific_device };
        }
        else
        {
            build_devices = this->_devices;
        }

        cl_int err = ret_program.build(build_devices, build_options.c_str());

        if(err != CL_SUCCESS)
        {
            StringD error_log = "Build failed for program:\n";
            error_log.appendf("OpenCL Error: {}", get_cl_error_string(err));

            for(const auto& device : this->_devices)
            {
                error_log.appendf("Device: {}", device.getInfo<CL_DEVICE_NAME>());

                try
                {
                    error_log.appendf("Build Log:\n{}",
                                        ret_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
                }
                catch(...)
                {
                    error_log.appendc("Failed to get build log for this device");
                }
            }

            log_error("OpenCL error: {}", error_log);

            return false;
        }

        {
            std::lock_guard<std::mutex> lock(this->_cache_mutex);

            if(this->_program_cache.size() >= this->_config.max_cache_size)
            {
                this->_program_cache.erase(this->_program_cache.begin());
            }

            this->_program_cache[cache_key] = ret_program;
        }

        return true;
    }

    STDROMANO_NO_DISCARD bool schedule_task(cl::Event& ret_event,
                                            const StringD& kernel_source,
                                            const StringD& kernel_name,
                                            std::function<void(cl::Kernel&)> set_kernel_args,
                                            const cl::NDRange& global_size,
                                            const cl::NDRange& local_size = cl::NullRange,
                                            const StringD& build_options = "",
                                            std::size_t device_index = 0,
                                            const std::vector<cl::Event>* wait_events = nullptr) noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(device_index >= this->_queues.size())
        {
            return false;
        }

        cl::CommandQueue& queue = this->_queues[device_index];
        const cl::Device& device = this->_devices[device_index];

        cl::Program program;

        if(!build_program_from_source(program, kernel_source, build_options, &device))
        {
            return false;
        }

        cl::Kernel kernel(program, kernel_name.c_str());

        set_kernel_args(kernel);

        cl_int err = queue.enqueueNDRangeKernel(kernel,
                                                cl::NullRange,
                                                global_size,
                                                local_size,
                                                wait_events,
                                                &ret_event);

        if(err != CL_SUCCESS)
        {
            log_error("Failed to schedule_task \"{}\": {}",
                        kernel_name,
                        this->get_cl_error_string(err));

            return false;
        }

        return true;
    }

    double get_execution_time_ms(const cl::Event& event) noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        if(!this->_config.enable_profiling)
        {
            return 0.0;
        }

        cl_int err = event.wait();

        if(err != CL_SUCCESS)
        {
            stdromano::log_error("Failed to get profiling info: {}", 
                                this->get_cl_error_string(err));
            return 0.0;
        }

        auto start = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        auto end = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
        return static_cast<double>(end - start) / 1e6;
    }

    void finish_all_queues() noexcept
    {
        STDROMANO_ASSERT(this->is_initialized(), "OpenCL Manager has not been initialized");

        for(auto& queue : this->_queues)
        {
            cl_int err = queue.finish();

            if(this->_config.enable_debug_output)
            {
                log_error("OpenCL error: failed to finish queue: {}",
                            this->get_cl_error_string(err));
            }
        }
    }

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

    const StringD& get_kernel_source(const StringD& name) noexcept
    {
        const StringD kernel_path = fs_expand_from_lib_dir(StringD("cl/{}.cl", name));

        if(!fs_path_exists(kernel_path))
        {
            static const StringD invalid;

            return invalid;
        }

        if(!this->_kernel_sources.contains(name))
        {
            this->_kernel_sources[name] = load_file_content(kernel_path, "r");
        }

        return this->_kernel_sources[name];
    }

    bool has_kernel_source(const StringD& name) const noexcept
    {
        if(this->_kernel_sources.contains(name))
        {
            return true;
        }
        
        const StringD kernel_path = fs_expand_from_lib_dir(StringD("cl/{}.cl", name));

        return fs_path_exists(kernel_path);
    }

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

    STDROMANO_NO_DISCARD bool setup_platform_and_devices() noexcept
    {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        if(platforms.empty())
        {
            log_error("OpenCL error: no platforms found");
            return false;
        }

        cl::Platform selected_platform;

        if(!this->_config.preferred_vendor.empty())
        {
            for(const auto& platform : platforms)
            {
                const std::string vendor = platform.getInfo<CL_PLATFORM_VENDOR>();

                if(vendor.find(this->_config.preferred_vendor.c_str()) != std::string::npos)
                {
                    selected_platform = platform;
                    break;
                }
            }
        }

        if(!selected_platform())
        {
            selected_platform = platforms[0];
        }

        this->_selected_platform = selected_platform;

        std::vector<cl::Device> all_devices;

        cl_int err = selected_platform.getDevices(this->_config.device_type,
                                                    &all_devices);

        if(err != CL_SUCCESS)
        {
            if(err == CL_DEVICE_NOT_FOUND)
            {
                log_error("OpenCL error: No devices of requested type found");
                return false;
            }

            log_error("OpenCL error: Failed to get devices: {}", 
                        this->get_cl_error_string(err));

            return false;
        }

        if(all_devices.empty())
        {
            log_error("OpenCL error: No suitable devices found");
            return false;
        }

        if(this->_config.prefer_dedicated_gpu && 
            this->_config.device_type == CL_DEVICE_TYPE_GPU)
        {
            std::sort(all_devices.begin(),
                        all_devices.end(),
                        [](const cl::Device& a, const cl::Device& b)
                        {
                            DeviceInfo info_a(a), info_b(b);

                            if(info_a.is_dedicated != info_b.is_dedicated)
                            {
                                return info_a.is_dedicated;
                            }
                            return info_a.global_mem_size >
                                    info_b.global_mem_size;
                        });
        }

        this->_devices = std::move(all_devices);

        this->_device_info.clear();

        for(const auto& device : this->_devices)
        {
            this->_device_info.emplace_back(device);
        }

        return true;
    }

    STDROMANO_NO_DISCARD bool create_context_and_queues() noexcept
    {
        cl_context_properties properties[] = {
            CL_CONTEXT_PLATFORM,
            reinterpret_cast<cl_context_properties>(this->_selected_platform()),
            0
        };

        /* TODO: Need to check error on that */
        this->_context = cl::Context(this->_devices, properties);

        this->_queues.clear();

        cl_command_queue_properties queue_props = 0;

        if(this->_config.enable_profiling)
        {
            queue_props |= CL_QUEUE_PROFILING_ENABLE;
        }

        for(const auto& device : this->_devices)
        {
            /* TODO: Need to check error on that */
            this->_queues.emplace_back(this->_context, device, queue_props);
        }

        return true;
    }

    void print_device_info() const noexcept
    {
        log_info("OpenCL Manager initialized with {} device(s):", this->_devices.size());

        for(std::size_t i = 0; i < this->_device_info.size(); ++i)
        {
            const auto& info = this->_device_info[i];

            log_info("Device {}: {} ({})", i, info.name, info.vendor);
            log_info("  Type: {}", this->get_device_type_string(info.type));
            log_info("  Global Memory: {} MB", (info.global_mem_size / (1024 * 1024)));
            log_info("  Compute Units: {}", info.compute_units);
            log_info("  Max Work Group Size: {}", info.max_work_group_size);
            log_info("  Dedicated: {}", (info.is_dedicated ? "Yes" : "No"));
            log_info("\n");
        }
    }

    StringD get_device_type_string(cl_device_type type) const noexcept
    {
        return StringD::make_ref(detail::get_cl_device_type_string(type));
    }

    StringD get_cl_error_string(cl_int error) const noexcept
    {
        const char* error_string = detail::get_cl_error_string(error);

        if(error_string == nullptr)
        {
            return StringD(static_cast<fmt::format_string<cl_int>>("Unknown error ({})"), error);
        }

        return StringD::make_ref(error_string);
    }

    cl::Platform _selected_platform;
    cl::Context _context;
    std::vector<cl::Device> _devices;
    std::vector<DeviceInfo> _device_info;
    std::vector<cl::CommandQueue> _queues;

    OpenCLConfig _config;

    HashMap<StringD, StringD> _kernel_sources;
    HashMap<ProgramCacheKey, cl::Program, ProgramCacheKeyHash> _program_cache;
    mutable std::mutex _cache_mutex;

    Atomic<size_t> _queue_counter{0};

    Atomic<bool> _initialized{false};
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

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/opencl.hpp"
#include "spdlog/fmt/bundled/base.h"

STDROMANO_NAMESPACE_BEGIN

OpenCLManager& OpenCLManager::get_instance() noexcept
{
    static OpenCLManager instance;
    return instance;
}

bool OpenCLManager::initialize(const OpenCLConfig& config) noexcept
{
    if(this->_initialized)
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

    this->_initialized = true;

    if(this->_config.enable_debug_output)
    {
        this->print_device_info();
    }

    return true;
}

bool OpenCLManager::build_program_from_source(cl::Program& ret_program,
                                              const StringD& source,
                                              const StringD& build_options,
                                              const cl::Device* specific_device) noexcept
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

bool OpenCLManager::schedule_task(cl::Event& ret_event,
                                  const StringD& kernel_source,
                                  const StringD& kernel_name,
                                  std::function<void(cl::Kernel&)> set_kernel_args,
                                  const cl::NDRange& global_size,
                                  const cl::NDRange& local_size,
                                  const StringD& build_options,
                                  std::size_t device_index,
                                  const std::vector<cl::Event>* wait_events) noexcept
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

double OpenCLManager::get_execution_time_ms(const cl::Event& event) noexcept
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

void OpenCLManager::finish_all_queues() noexcept
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

bool OpenCLManager::setup_platform_and_devices() noexcept
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

bool OpenCLManager::create_context_and_queues() noexcept
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

void OpenCLManager::print_device_info() const noexcept
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

StringD OpenCLManager::get_device_type_string(cl_device_type type) const noexcept
{
    switch(type)
    {
        case CL_DEVICE_TYPE_CPU:
            return StringD::make_ref("CPU");
        case CL_DEVICE_TYPE_GPU:
            return StringD::make_ref("GPU");
        case CL_DEVICE_TYPE_ACCELERATOR:
            return StringD::make_ref("Accelerator");
        case CL_DEVICE_TYPE_DEFAULT:
            return StringD::make_ref("Default");
        default:
            return StringD::make_ref("Unknown");
    }
}

StringD OpenCLManager::get_cl_error_string(cl_int error) const noexcept
{
    switch(error)
    {
        case CL_SUCCESS:
            return StringD::make_ref("Success");
        case CL_DEVICE_NOT_FOUND:
            return StringD::make_ref("Device not found");
        case CL_DEVICE_NOT_AVAILABLE:
            return StringD::make_ref("Device not available");
        case CL_COMPILER_NOT_AVAILABLE:
            return StringD::make_ref("Compiler not available");
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            return StringD::make_ref("Memory object allocation failure");
        case CL_OUT_OF_RESOURCES:
            return StringD::make_ref("Out of resources");
        case CL_OUT_OF_HOST_MEMORY:
            return StringD::make_ref("Out of host memory");
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            return StringD::make_ref("Profiling info not available");
        case CL_MEM_COPY_OVERLAP:
            return StringD::make_ref("Memory copy overlap");
        case CL_IMAGE_FORMAT_MISMATCH:
            return StringD::make_ref("Image format mismatch");
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            return StringD::make_ref("Image format not supported");
        case CL_BUILD_PROGRAM_FAILURE:
            return StringD::make_ref("Build program failure");
        case CL_MAP_FAILURE:
            return StringD::make_ref("Map failure");
        case CL_INVALID_VALUE:
            return StringD::make_ref("Invalid value");
        case CL_INVALID_DEVICE_TYPE:
            return StringD::make_ref("Invalid device type");
        case CL_INVALID_PLATFORM:
            return StringD::make_ref("Invalid platform");
        case CL_INVALID_DEVICE:
            return StringD::make_ref("Invalid device");
        case CL_INVALID_CONTEXT:
            return StringD::make_ref("Invalid context");
        case CL_INVALID_QUEUE_PROPERTIES:
            return StringD::make_ref("Invalid queue properties");
        case CL_INVALID_COMMAND_QUEUE:
            return StringD::make_ref("Invalid command queue");
        case CL_INVALID_HOST_PTR:
            return StringD::make_ref("Invalid host pointer");
        case CL_INVALID_MEM_OBJECT:
            return StringD::make_ref("Invalid memory object");
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            return StringD::make_ref("Invalid image format descriptor");
        case CL_INVALID_IMAGE_SIZE:
            return StringD::make_ref("Invalid image size");
        case CL_INVALID_SAMPLER:
            return StringD::make_ref("Invalid sampler");
        case CL_INVALID_BINARY:
            return StringD::make_ref("Invalid binary");
        case CL_INVALID_BUILD_OPTIONS:
            return StringD::make_ref("Invalid build options");
        case CL_INVALID_PROGRAM:
            return StringD::make_ref("Invalid program");
        case CL_INVALID_PROGRAM_EXECUTABLE:
            return StringD::make_ref("Invalid program executable");
        case CL_INVALID_KERNEL_NAME:
            return StringD::make_ref("Invalid kernel name");
        case CL_INVALID_KERNEL_DEFINITION:
            return StringD::make_ref("Invalid kernel definition");
        case CL_INVALID_KERNEL:
            return StringD::make_ref("Invalid kernel");
        case CL_INVALID_ARG_INDEX:
            return StringD::make_ref("Invalid argument index");
        case CL_INVALID_ARG_VALUE:
            return StringD::make_ref("Invalid argument value");
        case CL_INVALID_ARG_SIZE:
            return StringD::make_ref("Invalid argument size");
        case CL_INVALID_KERNEL_ARGS:
            return StringD::make_ref("Invalid kernel arguments");
        case CL_INVALID_WORK_DIMENSION:
            return StringD::make_ref("Invalid work dimension");
        case CL_INVALID_WORK_GROUP_SIZE:
            return StringD::make_ref("Invalid work group size");
        case CL_INVALID_WORK_ITEM_SIZE:
            return StringD::make_ref("Invalid work item size");
        case CL_INVALID_GLOBAL_OFFSET:
            return StringD::make_ref("Invalid global offset");
        case CL_INVALID_EVENT_WAIT_LIST:
            return StringD::make_ref("Invalid event wait list");
        case CL_INVALID_EVENT:
            return StringD::make_ref("Invalid event");
        case CL_INVALID_OPERATION:
            return StringD::make_ref("Invalid operation");
        case CL_INVALID_GL_OBJECT:
            return StringD::make_ref("Invalid GL object");
        case CL_INVALID_BUFFER_SIZE:
            return StringD::make_ref("Invalid buffer size");
        case CL_INVALID_MIP_LEVEL:
            return StringD::make_ref("Invalid mip level");
        case CL_INVALID_GLOBAL_WORK_SIZE:
            return StringD::make_ref("Invalid global work size");
        default:
            return StringD(static_cast<fmt::format_string<cl_int>>("Unknown error ({})"), error);
    }
}

STDROMANO_NAMESPACE_END
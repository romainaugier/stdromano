#if !defined(__STDROMANO_PROFILING)
#define __STDROMANO_PROFILING

#include "stdromano/cpu.h"
#include "stdromano/logger.h"

#include <chrono>
#include <ratio>
#include <type_traits>

STDROMANO_NAMESPACE_BEGIN

static STDROMANO_FORCE_INLINE uint64_t get_timestamp() noexcept
{
    return cpu_rdtsc();
}

static STDROMANO_FORCE_INLINE double get_elapsed_time(const uint64_t start,
                                                      const double unit_multiplier) noexcept
{
    return ((double)(cpu_rdtsc() - start) / ((double)cpu_get_current_frequency() * 1000000.0) *
            unit_multiplier);
}

namespace ProfileUnit
{
using Seconds = std::ratio<1, 1>;
using MilliSeconds = std::milli;
using MicroSeconds = std::micro;
using NanoSeconds = std::nano;
using Cycles = std::ratio<1, 3000000000>;
} // namespace ProfileUnit

template <typename T>
struct TypeName
{
    static constexpr const char* get_name()
    {
        return typeid(T).name();
    }
};

template <>
struct TypeName<ProfileUnit::Seconds>
{
    static const char* get_name()
    {
        return "seconds";
    }
};

template <>
struct TypeName<ProfileUnit::MilliSeconds>
{
    static const char* get_name()
    {
        return "ms";
    }
};

template <>
struct TypeName<ProfileUnit::MicroSeconds>
{
    static const char* get_name()
    {
#if defined(STDROMANO_WIN)
        return "µs"; /* Utf 8 encoding of microseconds */
#else
        return "µs";
#endif /* defined(STDROMANO_WIN) */
    }
};

template <>
struct TypeName<ProfileUnit::NanoSeconds>
{
    static const char* get_name()
    {
        return "ns";
    }
};

template <>
struct TypeName<ProfileUnit::Cycles>
{
    static const char* get_name()
    {
        return "cycles";
    }
};

/* Scoped profile */

template <typename Unit>
class ScopedProfile
{
    char* _name = nullptr;
    std::chrono::steady_clock::time_point _start;
    bool stopped = false;

public:
    ScopedProfile(const char* name)
    {
        this->_name = const_cast<char*>(name);

        this->_start = std::chrono::steady_clock::now();
    }

    ~ScopedProfile()
    {
        if(!stopped)
        {
            this->stop();
        }
    }

    STDROMANO_FORCE_INLINE void stop() noexcept
    {
        if(!stopped)
        {
            log_debug("Scoped profile \"{}\" -> {} {}",
                      this->_name,
                      std::chrono::duration_cast<std::chrono::duration<float, Unit>>(
                          std::chrono::steady_clock::now() - this->_start)
                          .count(),
                      TypeName<Unit>().get_name());
            this->stopped = true;
        }
    }
};

template <>
class ScopedProfile<ProfileUnit::Cycles>
{
    char* _name = nullptr;
    uint64_t _start = 0;
    bool stopped = false;

public:
    ScopedProfile(const char* name)
    {
        this->_name = const_cast<char*>(name);

        this->_start = cpu_rdtsc();
    }

    ~ScopedProfile()
    {
        if(!stopped)
        {
            this->stop();
        }
    }

    STDROMANO_FORCE_INLINE void stop() noexcept
    {
        if(!stopped)
        {
            log_debug("Scoped profile \"{}\" -> {} {}",
                      this->_name,
                      (uint64_t)(cpu_rdtsc() - this->_start),
                      TypeName<ProfileUnit::Cycles>().get_name());
            this->stopped = true;
        }
    }
};

/* Function profile */
template <typename Unit, typename F, typename... Args>
static auto __chrono_func_timer(const char* func_name, F&& func, Args&&... args) noexcept
{
    const auto start = std::chrono::steady_clock::now();

    auto ret = func(std::forward<Args>(args)...);

    log_debug("Func profile \"{}\" -> {} {}",
              func_name,
              std::chrono::duration_cast<std::chrono::duration<float, Unit>>(
                  std::chrono::steady_clock::now() - start)
                  .count(),
              TypeName<Unit>().get_name());

    return ret;
}

template <typename F, typename... Args>
static auto __cycles_func_timer(const char* func_name, F&& func, Args&&... args) noexcept
{
    const uint64_t start = cpu_rdtsc();

    auto ret = func(std::forward<Args>(args)...);

    log_debug("Func profile \"{}\" -> {} {}",
              func_name,
              (uint64_t)(cpu_rdtsc() - start),
              TypeName<ProfileUnit::Cycles>().get_name());

    return ret;
}

template <typename Unit, typename F, typename... Args>
static auto __func_timer(std::true_type, const char* func_name, F&& func, Args&&... args)
{
    return __cycles_func_timer(func_name, std::forward<F>(func), std::forward<Args>(args)...);
}

template <typename Unit, typename F, typename... Args>
static auto __func_timer(std::false_type, const char* func_name, F&& func, Args&&... args)
{
    return __chrono_func_timer<Unit>(func_name, std::forward<F>(func), std::forward<Args>(args)...);
}

template <typename Unit, typename F, typename... Args>
static auto _func_timer(const char* func_name, F&& func, Args&&... args)
{
    return __func_timer<Unit>(
        std::integral_constant<bool, std::is_same<Unit, ProfileUnit::Cycles>::value>{},
        func_name,
        std::forward<F>(func),
        std::forward<Args>(args)...);
}

STDROMANO_NAMESPACE_END

#if defined(STDROMANO_ENABLE_PROFILING)
#define SCOPED_PROFILE_START(profile_unit, name)                                                   \
    stdromano::ScopedProfile<profile_unit> __profile_##name(#name)
#define SCOPED_PROFILE_STOP(name) __profile_##name.stop()

#define PROFILE_FUNC(profile_unit, func, ...)                                                      \
    stdromano::_func_timer<profile_unit>(#func, func, ##__VA_ARGS__)

#define ___LERP_MEAN(X, Y, t)                                                                      \
    (static_cast<double>(X) +                                                                      \
     (static_cast<double>(Y) - static_cast<double>(X)) * static_cast<double>(t))
#define MEAN_PROFILE_INIT(name)                                                                    \
    const char* ___mean_##name = #name;                                                            \
    double ___mean_accum_##name = 0;                                                               \
    uint64_t ___mean_counter_##name = 0;
#define MEAN_PROFILE_START(name) uint64_t ___mean_start_##name = stdromano::get_timestamp();
#define MEAN_PROFILE_STOP(name)                                                                    \
    const double ___time_##name = stdromano::get_elapsed_time(___mean_start_##name, 1e9);          \
    ___mean_counter_##name++;                                                                      \
    const double ___t_##name = 1.0 / (double)___mean_counter_##name;                               \
    ___mean_accum_##name = ___LERP_MEAN(___mean_accum_##name, ___time_##name, ___t_##name);
#define MEAN_PROFILE_RELEASE(name)                                                                 \
    stdromano::log_debug("Mean profile \"{}\" -> {:.4} ns", ___mean_##name, ___mean_accum_##name);
#else
#define SCOPED_PROFILE_START(profile_unit, name)
#define SCOPED_PROFILE_STOP(name)

#define PROFILE_FUNC(profile_unit, func, ...) func(##__VA_ARGS__)

#define MEAN_PROFILE_INIT(name)
#define MEAN_PROFILE_START(name)
#define MEAN_PROFILE_STOP(name)
#define MEAN_PROFILE_RELEASE(name)
#endif /* defined(STDROMANO_ENABLE_PROFILING) */

#endif /* __STDROMANO_PROFILING */
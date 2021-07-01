module;
// Spdlog lib.
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
export module Log;

// This is what was missing from the program outside this module to compile.
// Using getters would not solve this "requirement". 
//export import <spdlog/sinks/stdout_color_sinks.h>;

// STL.
import <memory>;
import <utility>;

std::shared_ptr<spdlog::logger> g_CoreLogger;
std::shared_ptr<spdlog::logger> g_ClientLogger;

namespace Log
{
    export void Init()
    {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        g_CoreLogger = spdlog::stdout_color_mt("SEACREST");
        g_CoreLogger->set_level(spdlog::level::trace);

        g_ClientLogger = spdlog::stdout_color_mt("APP");
        g_ClientLogger->set_level(spdlog::level::trace);

        g_CoreLogger->info("Module core logger initialized.");
        g_ClientLogger->info("Module client logger initialized.");
    }
}

// Mocking spdlogs own implementation. 
// Perhaps an unnecessary layer, as we could namespace the actual logger.
// I.e. Log::g_CoreLogger->x. . .
// For now however, I'll take functions over macros. 
export
template<typename FormatString, typename... Args>
void CoreLoggerTrace(const FormatString& fmt, Args &&...args)
{
    g_CoreLogger->trace(fmt, std::forward<Args>(args)...);
}

export
template<typename FormatString, typename... Args>
void CoreLoggerInfo(const FormatString& fmt, Args &&...args)
{
    g_CoreLogger->info(fmt, std::forward<Args>(args)...);
}

export
template<typename FormatString, typename... Args>
void CoreLoggerWarn(const FormatString& fmt, Args &&...args)
{
    g_CoreLogger->warn(fmt, std::forward<Args>(args)...);
}

export
template<typename FormatString, typename... Args>
void CoreLoggerError(const FormatString& fmt, Args &&...args)
{
    g_CoreLogger->error(fmt, std::forward<Args>(args)...);
}

export
template<typename FormatString, typename... Args>
void CoreLoggerCritical(const FormatString& fmt, Args &&...args)
{
    g_CoreLogger->critical(fmt, std::forward<Args>(args)...);
}

export
template<typename FormatString, typename... Args>
void CoreLoggerDebug(const FormatString& fmt, Args &&...args)
{
    g_CoreLogger->debug(fmt, std::forward<Args>(args)...);
}
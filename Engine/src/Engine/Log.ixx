module;
#include <memory>
#include <utility>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
export module Log;

std::shared_ptr<spdlog::logger> s_CoreLogger;
std::shared_ptr<spdlog::logger> s_ClientLogger;

export inline std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
export inline std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

export void LogInit()
{
    spdlog::set_pattern("%^[%T] %n: %v%$");
    s_CoreLogger = spdlog::stdout_color_mt("SEACREST");
    s_CoreLogger->set_level(spdlog::level::trace);

    s_ClientLogger = spdlog::stdout_color_mt("APP");
    s_ClientLogger->set_level(spdlog::level::trace);

    s_CoreLogger->info("Module logger initialized.");
}

export
template<typename FormatString, typename... Args>
void Trace(const FormatString& fmt, Args &&...args)
{
    s_CoreLogger->trace(fmt, std::forward<Args>(args)...);
}
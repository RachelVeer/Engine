module;
#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
export module Log;

export class LogModule
{
public:
    void Init();

    std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
private:
    std::shared_ptr<spdlog::logger> s_CoreLogger;
    std::shared_ptr<spdlog::logger> s_ClientLogger;
};

//std::shared_ptr<spdlog::logger> LogModule::s_CoreLogger;
//std::shared_ptr<spdlog::logger> LogModule::s_ClientLogger;

void LogModule::Init()
{
    spdlog::set_pattern("%^[%T] %n: %v%$");
    s_CoreLogger = spdlog::stdout_color_mt("SEACREST2");
    s_CoreLogger->set_level(spdlog::level::trace);

    s_ClientLogger = spdlog::stdout_color_mt("APP2");
    s_ClientLogger->set_level(spdlog::level::trace);

    s_CoreLogger->trace("Module logger initialized.");
}

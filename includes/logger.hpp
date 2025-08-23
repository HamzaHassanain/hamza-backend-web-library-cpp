
#pragma once

#include <string>
#include <mutex>
#include <fstream>
namespace Logger
{
    std::string absolute_path_to_logs;
    extern const std::string INFO_LOGS_PATH;
    extern const std::string ERROR_LOGS_PATH;
    extern const std::string DEBUG_LOGS_PATH;
    extern const std::string TRACE_LOGS_PATH;
    extern const std::string FATAL_LOGS_PATH;
    extern std::mutex log_mutex;

    void LogInfo(const std::string &message);

    void LogError(const std::string &message);

    void LogDebug(const std::string &message);
    void LogTrace(const std::string &message);

    void LogFatal(const std::string &message);
}
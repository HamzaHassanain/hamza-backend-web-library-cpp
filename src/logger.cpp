
#include <string>
#include <mutex>
#include <fstream>
#include <logger.hpp>
namespace Logger
{
    std::string absolute_path_to_logs = "/home/hamza/Documents/Learnings/Projects/hamza-web-framwork/logs/";
    const std::string INFO_LOGS_PATH = absolute_path_to_logs + "info.log";
    const std::string ERROR_LOGS_PATH = absolute_path_to_logs + "error.log";
    const std::string DEBUG_LOGS_PATH = absolute_path_to_logs + "debug.log";
    const std::string TRACE_LOGS_PATH = absolute_path_to_logs + "trace.log";
    const std::string FATAL_LOGS_PATH = absolute_path_to_logs + "fatal.log";
    std::mutex log_mutex;

    void LogInfo(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream log_file(INFO_LOGS_PATH, std::ios::app);
        log_file << "[INFO] " << message << std::endl;
    }

    void LogError(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream log_file(ERROR_LOGS_PATH, std::ios::app);
        log_file << "[ERROR] " << message << std::endl;
    }

    void LogDebug(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream log_file(DEBUG_LOGS_PATH, std::ios::app);
        log_file << "[DEBUG] " << message << std::endl;
    }

    void LogTrace(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream log_file(TRACE_LOGS_PATH, std::ios::app);
        log_file << "[TRACE] " << message << std::endl;
    }

    void LogFatal(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream log_file(FATAL_LOGS_PATH, std::ios::app);
        log_file << "[FATAL] " << message << std::endl;
    }

}
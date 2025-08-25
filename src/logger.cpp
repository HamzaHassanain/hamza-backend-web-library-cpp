
#include <string>
#include <mutex>
#include <fstream>
#include "../includes/logger.hpp"

namespace hamza_web::logger
{
    std::string absolute_path_to_logs = "/path-you-want-to-your-logs/";

    bool enabled_logging = false;
    std::mutex log_mutex;

    void info(const std::string &message)
    {
        if (!enabled_logging)
            return;

        std::lock_guard<std::mutex> lock(log_mutex);
        const std::string INFO_LOGS_PATH = absolute_path_to_logs + "info.log";
        std::ofstream log_file(INFO_LOGS_PATH, std::ios::app);
        log_file << "[INFO] " << message << std::endl;
    }

    void error(const std::string &message)
    {
        if (!enabled_logging)
            return;
        std::lock_guard<std::mutex> lock(log_mutex);
        const std::string ERROR_LOGS_PATH = absolute_path_to_logs + "error.log";
        std::ofstream log_file(ERROR_LOGS_PATH, std::ios::app);
        log_file << "[ERROR] " << message << std::endl;
    }

    void debug(const std::string &message)
    {
        if (!enabled_logging)
            return;
        std::lock_guard<std::mutex> lock(log_mutex);
        const std::string DEBUG_LOGS_PATH = absolute_path_to_logs + "debug.log";
        std::ofstream log_file(DEBUG_LOGS_PATH, std::ios::app);
        log_file << "[DEBUG] " << message << std::endl;
    }

    void trace(const std::string &message)
    {
        if (!enabled_logging)
            return;
        std::lock_guard<std::mutex> lock(log_mutex);
        const std::string TRACE_LOGS_PATH = absolute_path_to_logs + "trace.log";
        std::ofstream log_file(TRACE_LOGS_PATH, std::ios::app);
        log_file << "[TRACE] " << message << std::endl;
    }

    void fatal(const std::string &message)
    {
        if (!enabled_logging)
            return;
        std::lock_guard<std::mutex> lock(log_mutex);
        const std::string FATAL_LOGS_PATH = absolute_path_to_logs + "fatal.log";
        std::ofstream log_file(FATAL_LOGS_PATH, std::ios::app);
        log_file << "[FATAL] " << message << std::endl;
    }

    void clear()
    {
        if (!enabled_logging)
            return;

        std::lock_guard<std::mutex> lock(log_mutex);

        std::ofstream(absolute_path_to_logs + "info.log", std::ios::trunc).close();
        std::ofstream(absolute_path_to_logs + "error.log", std::ios::trunc).close();
        std::ofstream(absolute_path_to_logs + "debug.log", std::ios::trunc).close();
        std::ofstream(absolute_path_to_logs + "trace.log", std::ios::trunc).close();
        std::ofstream(absolute_path_to_logs + "fatal.log", std::ios::trunc).close();
    }

}

#pragma once

#include <string>
#include <mutex>
#include <fstream>

/**
 * @brief Logger for Web server events and errors.
 *
 * This logger provides a simple interface for logging messages related to
 * the Web server's operation, including request handling, error reporting,
 * and other important events.
 */

namespace hh_web::logger
{

    /**
     * @brief Absolute path to the logs directory.
     *
     * This path must be valid and must end with a directory separator (e.g., /some/path/logs/ or C:\some\path\logs\).
     */
    extern std::string absolute_path_to_logs;

    /// @brief Flag to enable or disable logging, default is false
    extern bool enabled_logging;
    extern std::mutex log_mutex;

    /// @brief Log an informational message to a file called "info.log"
    /// @param message The message to log
    void info(const std::string &message);

    /// @brief Log an error message to a file called "error.log"
    /// @param message The message to log
    void error(const std::string &message);

    /// @brief Log a debug message to a file called "debug.log"
    /// @param message The message to log
    void debug(const std::string &message);

    /// @brief Log a trace message to a file called "trace.log"
    /// @param message The message to log
    void trace(const std::string &message);

    /// @brief Log a fatal message to a file called "fatal.log"
    /// @param message The message to log
    void fatal(const std::string &message);

    /// @brief Clear all log files
    void clear();
}
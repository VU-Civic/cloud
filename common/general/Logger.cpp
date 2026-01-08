#include <stdarg.h>
#include <time.h>
#include "Logger.h"

// Static local log level strings
static const char* const log_level_strings[] = {"ERROR", "WARNING", "INFO", "DEBUG"};

Logger::Logger(const char* __restrict log_file_path, LogLevel max_log_level) : max_log_level(max_log_level), time_string{0}, log_file(fopen(log_file_path, "a+"))
{
  // Log an application start message
  log(INFO, "Application starting...\n");
}

Logger::~Logger(void)
{
  // Ensure that the log file is closed
  fclose(log_file);
}

void Logger::log(LogLevel log_level, const char* __restrict fmt, ...)
{
  // Only log if this message has a high enough level
  if (log_level <= max_log_level)
  {
    // Print the log level and current timestamp
    time_t curr_time = time(nullptr);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", gmtime(&curr_time));
    fprintf(log_file, "[%s] %s: ", time_string, log_level_strings[log_level]);

    // Generate a variadic argument to pass to the system print function
    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    // Flush the file to ensure that the log message is written immediately
    fflush(log_file);
  }
}

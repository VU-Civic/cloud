#include <stdarg.h>
#include <time.h>
#include "Logger.h"

// Static local log level strings
static const char* const logLevelStrings[] = {"ERROR", "WARNING", "INFO", "DEBUG"};

Logger::Logger(const char* logFilePath, LogLevel maxLogLevel) : inUse(false), logPath(logFilePath), maxLogLevel(maxLogLevel), timeString{0}, logFile(fopen(logFilePath, "a"))
{
  // Log an application start message
  log(INFO, "Application starting...\n");
}

Logger::~Logger(void)
{
  // Ensure that the log file is closed
  while (inUse.test_and_set(std::memory_order_acquire));
  fclose(logFile);
  inUse.clear(std::memory_order_release);
}

void Logger::log(LogLevel logLevel, const char* __restrict fmt, ...)
{
  // Only log if this message has a high enough level
  while (inUse.test_and_set(std::memory_order_acquire));
  if (logLevel <= maxLogLevel)
  {
    // Print the log level and current timestamp
    time_t currTime = time(nullptr);
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", gmtime(&currTime));
    fprintf(logFile, "[%s] %s: ", timeString, logLevelStrings[logLevel]);

    // Generate a variadic argument to pass to the system print function
    va_list args;
    va_start(args, fmt);
    vfprintf(logFile, fmt, args);
    va_end(args);

    // Flush the file to ensure that the log message is written immediately
    fflush(logFile);
  }
  inUse.clear(std::memory_order_release);
}

void Logger::rotate(const char* oldLogFileNewPath)
{
  // Rotate the log file to a new file path
  while (inUse.test_and_set(std::memory_order_acquire));
  fclose(logFile);
  rename(logPath.c_str(), oldLogFileNewPath);
  logFile = fopen(logPath.c_str(), "w");
  inUse.clear(std::memory_order_release);
}

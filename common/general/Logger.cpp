#include <stdarg.h>
#include <time.h>
#include "Logger.h"

// Static local log level strings
static const char* const logLevelStrings[] = {"ERROR", "WARNING", "INFO", "DEBUG"};

Logger::Logger(const char* logFilePath, LogLevel maxLogLevel)
    : inUse(false),
      logPath(logFilePath),
      maxLogLevel(maxLogLevel),
      timeString{0},
      logFile(fopen(logFilePath, "a")),
      rotationThread(),
      rotationRunning(false),
      terminateRotation(),
      rotationPath(),
      rotationInterval(0)
{
  // Log an application start message
  log(INFO, "Application starting...\n");
}

Logger::~Logger(void)
{
  // Ensure that log file rotation has stopped
  disableRotation();

  // Close the log file
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

void Logger::enableRotation(const char* rotationDestination, uint32_t rotationIntervalSeconds)
{
  // Start a thread to handle log file rotation
  if (!rotationRunning.load(std::memory_order_relaxed))
  {
    rotationInterval = std::max(rotationIntervalSeconds, 300U);
    rotationRunning.store(true, std::memory_order_relaxed);
    rotationPath = std::string(rotationDestination);
    rotationThread = std::thread(&Logger::logRotationWorker, this);
  }
}

void Logger::disableRotation(void)
{
  // Ensure that log rotation has stopped
  while (inUse.test_and_set(std::memory_order_acquire));
  rotationRunning.store(false, std::memory_order_relaxed);
  if (rotationThread.joinable())
  {
    terminateRotation.notify_one();
    rotationThread.join();
  }
  inUse.clear(std::memory_order_release);
}

void Logger::rotate(const char* newPath)
{
  // Rotate the log file to a new file path
  while (inUse.test_and_set(std::memory_order_acquire));
  fclose(logFile);
  rename(logPath.c_str(), newPath);
  logFile = fopen(logPath.c_str(), "w");
  inUse.clear(std::memory_order_release);
}

void Logger::logRotationWorker(void)
{
  // Loop forever until termination requested
  while (rotationRunning.load(std::memory_order_relaxed))
  {
    // Sleep for the log rotation interval
    std::mutex sleepMutex;
    std::unique_lock<std::mutex> lock(sleepMutex);
    terminateRotation.wait_for(lock, std::chrono::seconds(rotationInterval), [this]() { return !rotationRunning.load(std::memory_order_relaxed); });

    // Rotate the current log file to a new location based on the current timestamp
    if (rotationRunning.load(std::memory_order_relaxed))
    {
      std::string newLogFilePath = rotationPath + std::to_string(time(nullptr)) + std::string(".log");
      rotate(newLogFilePath.c_str());
    }
  }
}

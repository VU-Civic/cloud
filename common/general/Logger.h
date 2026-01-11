#ifndef __LOGGER_HEADER_H__
#define __LOGGER_HEADER_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

class Logger final
{
public:

  // Log level type enumeration
  enum LogLevel
  {
    ERROR = 0,
    WARNING,
    INFO,
    DEBUG
  };

  // Construction and destruction
  Logger(const char* logFilePath, LogLevel maxLogLevel);
  ~Logger(void);

  // Logging functionality
  void log(LogLevel logLevel, const char* __restrict fmt, ...);
  void enableRotation(const char* rotationDestination, uint32_t rotationIntervalSeconds);
  void disableRotation(void);

private:

  // Log rotation functions
  void rotate(const char* newPath);
  void logRotationWorker(void);

  // Private logging variables
  std::atomic_flag inUse;
  std::string logPath;
  LogLevel maxLogLevel;
  char timeString[25];
  FILE* logFile;

  // Private rotation variables
  std::thread rotationThread;
  std::atomic<bool> rotationRunning;
  std::condition_variable terminateRotation;
  std::string rotationPath;
  uint32_t rotationInterval;
};

#endif  // #ifndef __LOGGER_HEADER_H__

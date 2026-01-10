#ifndef __LOGGER_HEADER_H__
#define __LOGGER_HEADER_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

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
  void rotate(const char* oldLogFileNewPath);

private:

  // Private variables
  std::atomic_flag inUse;
  std::string logPath;
  LogLevel maxLogLevel;
  char timeString[25];
  FILE* logFile;
};

#endif  // #ifndef __LOGGER_HEADER_H__

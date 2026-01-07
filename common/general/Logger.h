#ifndef __LOGGER_HEADER_H__
#define __LOGGER_HEADER_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
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
  Logger(const char* __restrict log_file_path, LogLevel max_log_level);
  ~Logger(void);

  // Logging functionality
  void log(LogLevel log_level, const char* __restrict fmt, ...);

private:

  // Private variables
  LogLevel max_log_level;
  char time_string[25];
  FILE* log_file;
};

#endif  // #ifndef __LOGGER_HEADER_H__

#ifndef __PROCESS_UTILS_HEADER_H__
#define __PROCESS_UTILS_HEADER_H__

class ProcessUtils final
{
public:

  static void setupSignalHandlers(void);
  static void runUntilTermination(void);

private:

  static void* generalSignalHandler(void* args);

  static sigset_t signalMask;
  static pthread_t signalHandlerThread;
};

#endif  // #ifndef __PROCESS_UTILS_HEADER_H__

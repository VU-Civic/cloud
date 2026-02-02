#include <csignal>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include "ProcessUtils.h"

// Global process utility variables
sigset_t ProcessUtils::signalMask;
pthread_t ProcessUtils::signalHandlerThread;

void* ProcessUtils::generalSignalHandler(void*)
{
  // Loop forever catching signals
  while (true)
  {
    // Wait until one of the masked signals is caught
    int signalCaught = 0;
    if (sigwait(&signalMask, &signalCaught) != 0) continue;

    // Handle the received signal
    switch (signalCaught)
    {
      case SIGTERM:
      case SIGINT:
      case SIGQUIT:
      case SIGHUP:
        pthread_exit(nullptr);
        break;
      case SIGUSR1:
      case SIGUSR2:
      default:
        break;
    }
  }
  return 0;
}

void ProcessUtils::setupSignalHandlers(void)
{
  // Initialize a signal mask with signals of interest
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGUSR1);
  sigaddset(&signalMask, SIGUSR2);
  sigaddset(&signalMask, SIGTERM);
  sigaddset(&signalMask, SIGINT);
  sigaddset(&signalMask, SIGQUIT);
  sigaddset(&signalMask, SIGHUP);

  // Block the masked signals on the current thread
  if (pthread_sigmask(SIG_BLOCK, &signalMask, nullptr)) exit(EXIT_FAILURE);

  // Create a new thread to handle termination signals
  pthread_attr_t signalHandlerAttributes;
  pthread_attr_init(&signalHandlerAttributes);
  pthread_attr_setdetachstate(&signalHandlerAttributes, PTHREAD_CREATE_JOINABLE);
  if (pthread_create(&signalHandlerThread, &signalHandlerAttributes, &generalSignalHandler, nullptr)) exit(EXIT_FAILURE);
  pthread_attr_destroy(&signalHandlerAttributes);
}

void ProcessUtils::runUntilTermination(void)
{
  // Wait for the signal handler thread to terminate
  pthread_join(signalHandlerThread, nullptr);
  pthread_sigmask(SIG_UNBLOCK, &signalMask, nullptr);
}

#include "AwsServices.h"
#include "Common.h"
#include "FusionAlgorithm.h"
#include "PacketReceiver.h"
#include "ProcessUtils.h"

// Global application logger
Logger logger(CivicAlert::PROCESS_LOG_FILE, CivicAlert::LOG_MAX_LEVEL);

// Cleanup routine to be called upon application termination
static void globalCleanup(void)
{
  // Stop all running tasks
  logger.log(Logger::INFO, "Application shutting down...\n");
  PacketReceiver::stopListening();

  // Clean up any libraries that may have been initialized
  Fusion::cleanup();
  AwsServices::cleanup();
  logger.log(Logger::INFO, "Application successfully shut down!\n\n");
}

int main(void)
{
  // Set up application signal handlers and a global cleanup routine
  ProcessUtils::setupSignalHandlers();
  atexit(globalCleanup);

  // Initialize all necessary AWS services and the fusion pipeline
  AwsServices::initialize();
  Fusion::initialize();

  // Enable log file rotation
  logger.enableRotation(CivicAlert::LOG_FILE_ROTATION_DIRECTORY, CivicAlert::LOG_FILE_ROTATION_INTERVAL_SECONDS);

  // Subscribe to incoming alert packets over MQTT
  logger.log(Logger::INFO, "Subscribing to the MQTT alerts topic...\n");
  if (!AwsServices::mqttConnect()) exit(EXIT_FAILURE);
  if (!AwsServices::mqttSubscribe(AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_ALERTS_TOPIC).c_str(), 1)) exit(EXIT_FAILURE);
  logger.log(Logger::INFO, "MQTT topic subscription complete\n");

  // Listen for incoming MQTT packets until a termination signal is received
  PacketReceiver::listenForPackets();
  ProcessUtils::runUntilTermination();

  // Return success
  return EXIT_SUCCESS;
}

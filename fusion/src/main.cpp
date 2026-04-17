#include "AwsServices.h"
#include "Common.h"
#include "FusionAlgorithm.h"
#include "PacketReceiver.h"
#include "ProcessUtils.h"
#include "WeatherRetriever.h"

// Global application logger
Logger logger(CivicAlert::PROCESS_LOG_FILE, Logger::LogLevel::INFO);

// Cleanup routine to be called upon application termination
static void globalCleanup(void)
{
  // Stop all running tasks
  logger.log(Logger::INFO, "Application shutting down...\n");
  PacketReceiver::stopListening();

  // Clean up any libraries that may have been initialized
  Fusion::cleanup();
  WeatherDataRetriever::uninitialize();
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
  WeatherDataRetriever::initialize();
  Fusion::initialize(AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_WEATHER_OPENWEATHERMAP_API_ID).c_str(),
                     AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_WEATHER_TOMORROWIO_API_ID).c_str());

  // Enable log file rotation
  logger.enableRotation(CivicAlert::LOG_FILE_ROTATION_DIRECTORY, CivicAlert::LOG_FILE_ROTATION_INTERVAL_SECONDS);

  // Subscribe to incoming device info and alert packets over MQTT
  logger.log(Logger::INFO, "Subscribing to the MQTT device info and alerts topics...\n");
  if (!AwsServices::mqttConnect()) exit(EXIT_FAILURE);
  const auto mqttDeviceInfoTopic = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_DEVICE_INFO_TOPIC);
  const auto mqttAlertsTopic = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_ALERTS_TOPIC);
  if (!AwsServices::mqttSubscribe(mqttDeviceInfoTopic.c_str(), 1)) exit(EXIT_FAILURE);
  if (!AwsServices::mqttSubscribe(mqttAlertsTopic.c_str(), 1)) exit(EXIT_FAILURE);
  logger.log(Logger::INFO, "MQTT topic subscription complete\n");

  // Listen for incoming MQTT packets until a termination signal is received
  PacketReceiver::listenForPackets(mqttDeviceInfoTopic.c_str(), mqttAlertsTopic.c_str());
  ProcessUtils::runUntilTermination();

  // Return success
  return EXIT_SUCCESS;
}

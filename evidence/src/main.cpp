#include "AwsServices.h"
#include "Common.h"
#include "EvidenceProcessor.h"
#include "PacketReceiver.h"
#include "ProcessUtils.h"

// Global application logger
Logger logger(CivicAlert::PROCESS_LOG_FILE, CivicAlert::LOG_MAX_LEVEL);

// Cleanup routine to be called upon application termination
void globalCleanup(void)
{
  // Stop all running tasks
  logger.log(Logger::INFO, "Application shutting down...\n");
  PacketReceiver::stopListening();

  // Clean up any libraries that may have been initialized
  EvidenceProcessor::cleanup();
  AwsServices::cleanup();
  logger.log(Logger::INFO, "Application successfully shut down!\n\n");
}

int main(void)
{
  // Set up application signal handlers and a global cleanup routine
  ProcessUtils::setupSignalHandlers();
  atexit(globalCleanup);

  // Initialize all necessary AWS services and the evidence processing pipeline
  AwsServices::initialize();
  EvidenceProcessor::initialize();

  // Subscribe to incoming audio packets over MQTT
  logger.log(Logger::INFO, "Subscribing to the MQTT evidence topic...");
  if (!AwsServices::mqttConnect()) exit(EXIT_FAILURE);
  if (!AwsServices::mqttSubscribe(AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_AUDIO_EVIDENCE_TOPIC).c_str(), 1)) exit(EXIT_FAILURE);
  logger.log(Logger::INFO, "MQTT topic subscription complete");

  // Listen for incoming MQTT packets until a termination signal is received
  PacketReceiver::listenForPackets();
  ProcessUtils::runUntilTermination();

  // Return success
  return EXIT_SUCCESS;
}

// TODO: Use CloudWatch (https://docs.aws.amazon.com/AmazonCloudWatch/latest/monitoring/CloudWatch-Agent-Configuration-File-Details.html)
// TODO: Ensure this is started with a process monitor to automatically restart on failure

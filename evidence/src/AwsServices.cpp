#include "AwsServices.h"
#include "Common.h"

int AwsServices::referenceCount = 0;
std::mutex AwsServices::initializationMutex;
std::unique_ptr<AwsS3> AwsServices::s3Client = nullptr;
std::unique_ptr<AwsMQTT> AwsServices::mqttClient = nullptr;
std::unique_ptr<AwsSecrets> AwsServices::secretManager = nullptr;

void AwsServices::initialize(void)
{
  // Only allow one complete initialization
  std::lock_guard<std::mutex> lock(initializationMutex);
  if (referenceCount++ > 0) return;

  // Initialize the AWS SDK and create a secrets manager
  logger.log(Logger::INFO, "Configuring AWS clients...\n");
  AWS::initialize();
  secretManager = std::make_unique<AwsSecrets>();

  // Retrieve all necessary AWS parameters and secrets
  std::string mqttCredentialsKey(secretManager->getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CREDENTIALS_KEY));
  std::string mqttCAKey(secretManager->getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CA_KEY));
  std::string mqttClientCertKey(secretManager->getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CLIENT_CERT_KEY));
  std::string mqttClientKeyKey(secretManager->getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CLIENT_KEY_KEY));
  const auto mqttCredentials = secretManager->getSecret(mqttCredentialsKey.c_str());
  std::string mqttCa(secretManager->extractSecretValue(mqttCredentials, mqttCAKey.c_str()));
  std::string mqttClientCert(secretManager->extractSecretValue(mqttCredentials, mqttClientCertKey.c_str()));
  std::string mqttClientKey(secretManager->extractSecretValue(mqttCredentials, mqttClientKeyKey.c_str()));
  std::string mqttEndpoint(secretManager->getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_ENDPOINT).c_str());
  std::string mqttPort(secretManager->getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_PORT).c_str());
  std::string evidenceBucket(secretManager->getParameter(CivicAlert::AWS_PARAMETER_KEY_S3_EVIDENCE_BUCKET));

  // Ensure that all required parameters and secrets were retrieved
  if (mqttCa.empty() || mqttClientCert.empty() || mqttClientKey.empty() || mqttEndpoint.empty() || mqttPort.empty() || evidenceBucket.empty())
  {
    logger.log(Logger::ERROR, "AWS client configuration failed due to missing parameters or secrets\n");
    exit(EXIT_FAILURE);
  }

  // Initialize the AWS S3 and MQTT clients
  s3Client = std::make_unique<AwsS3>(evidenceBucket.c_str());
  mqttClient = std::make_unique<AwsMQTT>(CivicAlert::MQTT_EVIDENCE_CLIENT_ID, mqttCa.c_str(), mqttClientCert.c_str(), mqttClientKey.c_str(), mqttEndpoint.c_str(),
                                         std::stoi(mqttPort.c_str()), CivicAlert::MQTT_KEEP_ALIVE_SECONDS);
  logger.log(Logger::INFO, "AWS clients successfully configured\n");
}

void AwsServices::cleanup(void)
{
  // Only allow one complete cleanup
  std::lock_guard<std::mutex> lock(initializationMutex);
  if (--referenceCount > 0) return;
  if (referenceCount < 0)
  {
    // Disallow unmatched uninitializations
    referenceCount = 0;
    return;
  }

  // Cleanup AWS SDK and clients
  s3Client.reset();
  mqttClient.reset();
  secretManager.reset();
  AWS::uninitialize();
}

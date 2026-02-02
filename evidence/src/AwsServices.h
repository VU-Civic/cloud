#ifndef __AWS_SERVICES_HEADER_H__
#define __AWS_SERVICES_HEADER_H__

#include <mutex>
#include "mqtt.h"
#include "s3.h"
#include "secrets.h"

class AwsServices final
{
public:

  // Initialization/cleanup
  static void initialize(void);
  static void cleanup(void);

  // Accessors for AWS service clients
  static rapidjson::Document getSecret(const char* __restrict secretName) { return secretManager->getSecret(secretName); }
  static std::string getSecretParameter(const char* __restrict parameterName) { return secretManager->getParameter(parameterName); }
  static std::string extractSecretValue(const rapidjson::Document& secretJson, const char* __restrict valueKey) { return secretManager->extractSecretValue(secretJson, valueKey); }
  static inline bool mqttConnect(void) { return mqttClient->connect(); }
  static inline void mqttDisconnect(void) { mqttClient->disconnect(); }
  static inline bool mqttSubscribe(const char* __restrict topic, uint8_t qos) { return mqttClient->subscribe(topic, qos); }
  static inline void mqttUnsubscribe(const char* __restrict topic) { mqttClient->unsubscribe(topic); }
  static inline std::vector<uint8_t> mqttReceive(void) { return mqttClient->receive(); }
  static inline bool storeEvidenceClipToS3(const char* __restrict s3FileName, const char* __restrict inputFilePath) { return s3Client->storeFile(s3FileName, inputFilePath); }

private:

  // Private member variables
  static int referenceCount;
  static std::mutex initializationMutex;
  static std::unique_ptr<AwsSecrets> secretManager;
  static std::unique_ptr<AwsS3> s3Client;
  static std::unique_ptr<AwsMQTT> mqttClient;
};

#endif  // #ifndef __AWS_SERVICES_HEADER_H__

#include <curl/curl.h>
#include "Common.h"
#include "mqtt.h"
#include "s3.h"
#include "secrets.h"
#include "sqs.h"

// Global application logger
Logger logger("/tmp/civicalert.log", Logger::DEBUG);

static size_t writeCurlDataString(void* downloadedData, size_t wordSize, size_t numWords, void* outputStringPtr)
{
  const auto outputSize = wordSize * numWords;
  static_cast<std::string*>(outputStringPtr)->append(static_cast<char*>(downloadedData), outputSize);
  return outputSize;
}

void testSecrets(void)
{
  AwsSecrets secretManager;
  std::string mqttCredentialsKey(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CREDENTIALS_KEY));
  printf("MQTT_CREDENTIALS_KEY: %s\n", mqttCredentialsKey.c_str());
  std::string mqttCAKey(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CA_KEY));
  printf("MQTT_CA_KEY: %s\n", mqttCAKey.c_str());
  std::string mqttClientCertKey(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CLIENT_CERT_KEY));
  printf("MQTT_CLIENT_CERT_KEY: %s\n", mqttClientCertKey.c_str());
  std::string mqttClientKeyKey(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CLIENT_KEY_KEY));
  printf("MQTT_CLIENT_KEY_KEY: %s\n", mqttClientKeyKey.c_str());
  printf("MQTT_DEVICE_INFO_TOPIC: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_DEVICE_INFO_TOPIC).c_str());
  printf("MQTT_ALERTS_TOPIC: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_ALERTS_TOPIC).c_str());
  printf("MQTT_AUDIO_EVIDENCE_TOPIC: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_AUDIO_EVIDENCE_TOPIC).c_str());
  printf("MQTT_ENDPOINT: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_ENDPOINT).c_str());
  printf("MQTT_PORT: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_PORT).c_str());
  printf("S3_EVIDENCE_BUCKET: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_S3_EVIDENCE_BUCKET).c_str());
  printf("SQS_DATA_QUEUE_URL: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_SQS_DATA_QUEUE_URL).c_str());
  printf("SNS_ALERT_TOPIC_ARN: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_SNS_ALERT_TOPIC_ARN).c_str());
  printf("WEATHER_OPENWEATHERMAP_API_ID: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_WEATHER_OPENWEATHERMAP_API_ID).c_str());
  printf("WEATHER_TOMORROWIO_API_ID: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_WEATHER_TOMORROWIO_API_ID).c_str());
  const auto mqttCredentials = secretManager.getSecret(mqttCredentialsKey.c_str());
  printf("MQTT_CA: %s\n", secretManager.extractSecretValue(mqttCredentials, mqttCAKey.c_str()).c_str());
  printf("MQTT_CLIENT_CERT: %s\n", secretManager.extractSecretValue(mqttCredentials, mqttClientCertKey.c_str()).c_str());
  printf("MQTT_CLIENT_KEY: %s\n", secretManager.extractSecretValue(mqttCredentials, mqttClientKeyKey.c_str()).c_str());
  printf("API_ENDPOINT_URL: %s\n", secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_URL).c_str());
  std::string apiEndpointTokenKey(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_TOKEN_KEY));
  printf("API_ENDPOINT_TOKEN_KEY: %s\n", apiEndpointTokenKey.c_str());
  const auto apiEndpointTokenSecret = secretManager.getSecret(apiEndpointTokenKey.c_str());
  printf("API_ENDPOINT_TOKEN_SECRET: %s\n", secretManager.extractSecretValue(apiEndpointTokenSecret, "token").c_str());
}

void testApiEndpoint(void)
{
  // Retrieve the API endpoint URL and bearer token from AWS secrets
  AwsSecrets secretManager;
  std::string apiEndpointUrl(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_URL));
  std::string apiEndpointTokenKey(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_TOKEN_KEY));
  const auto apiEndpointTokenSecret = secretManager.getSecret(apiEndpointTokenKey.c_str());
  std::string apiEndpointToken = secretManager.extractSecretValue(apiEndpointTokenSecret, "token");

  // Initialize the cURL library and set default options for all future cURL requests
  void* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "CivicAlertTest/1.0");
  curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");

  // Attempt to issue a "whoami" request to the API endpoint with the retrieved URL and token
  std::string output;
  auto headers = curl_slist_append(nullptr, "Content-Type: application/json");
  headers = curl_slist_append(headers, (std::string("Authorization: Token ") + apiEndpointToken).c_str());
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCurlDataString);
  curl_easy_setopt(curl, CURLOPT_URL, (apiEndpointUrl + "/auth/whoami").c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  if (curl_easy_perform(curl) == CURLE_OK)
    printf("API WHOAMI Endpoint Response: %s\n", output.c_str());
  else
    printf("API WHOAMI Endpoint request failed\n");

  // Clean up the cURL library
  if (curl)
  {
    curl_easy_cleanup(curl);
    curl = nullptr;
  }
}

void testS3(const char* s3BucketName)
{
  AwsS3 s3Client(s3BucketName);
  printf("S3 Connected: %s\n", s3Client.isConnected() ? "Yes" : "No");
  printf("Storing in-memory file to S3: %s\n", s3Client.storeFile("test/txt.txt", (uint8_t*)"ABCD\0", 5) ? "Success" : "Failure");
  printf("Storing on-disk file to S3: %s\n", s3Client.storeFile("test/gunshot1.wav", "assets/gunshot1.wav") ? "Success" : "Failure");
  printf("S3 File Size: %zu bytes\n", s3Client.getFileSize("test/gunshot1.wav"));
  printf("Listing files in S3 bucket:\n");
  const auto bucketFiles = s3Client.listFilesAndSizes();
  for (const auto& [fileName, fileSize] : bucketFiles) printf("  %s (%zu bytes)\n", fileName.c_str(), fileSize);
  printf("Deleting single file from S3: %s\n", s3Client.deleteFile("test/txt.txt") ? "Success" : "Failure");
  printf("Deleting single file from S3: %s\n", s3Client.deleteFile("test/gunshot1.wav") ? "Success" : "Failure");
}

void testSQS(const char* queueURL)
{
  AwsSQS sqsClient(queueURL);
  sqsClient.purgeQueue();
  printf("SQS Sending message: %s\n", sqsClient.sendMessage("Test message sent from CivicAlert cloud-c") ? "Success" : "Failure");
  auto [received, receiptHandle, messageBody] = sqsClient.receiveMessage(5);
  if (received)
  {
    printf("SQS Received message: %s\n", messageBody.c_str());
    printf("SQS Deleting message: %s\n", sqsClient.deleteMessage(receiptHandle.c_str()) ? "Success" : "Failure");
  }
  else
    printf("SQS No message received\n");
}

void testMQTT(void)
{
  // Initialize the AWS MQTT client
  AwsSecrets secretManager;
  std::string evidenceTopic(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_AUDIO_EVIDENCE_TOPIC));
  const auto mqttCredentials = secretManager.getSecret(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CREDENTIALS_KEY).c_str());
  AwsMQTT mqtt(CivicAlert::MQTT_CLOUD_CLIENT_ID, secretManager.extractSecretValue(mqttCredentials, secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CA_KEY).c_str()),
               secretManager.extractSecretValue(mqttCredentials, secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CLIENT_CERT_KEY).c_str()),
               secretManager.extractSecretValue(mqttCredentials, secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_CLIENT_KEY_KEY).c_str()),
               secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_ENDPOINT).c_str(),
               std::stoi(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_MQTT_PORT).c_str()), CivicAlert::MQTT_KEEP_ALIVE_SECONDS);

  // Connect to AWS MQTT and subscribe to the audio evidence topic
  if (!mqtt.connect())
  {
    printf("MQTT Connection failed\n");
    return;
  }
  printf("MQTT Connected\n");
  if (!mqtt.subscribe(evidenceTopic.c_str(), 1))
  {
    printf("MQTT Subscription failed\n");
    return;
  }
  printf("MQTT Subscribed to audio evidence topic\n");

  // Publish a test message to the audio evidence topic
  const char* testPayload = "Test audio evidence message from CivicAlert cloud-c";
  if (!mqtt.publish(evidenceTopic.c_str(), (const uint8_t*)testPayload, 1 + strlen(testPayload), 1, false))
  {
    printf("MQTT Publish failed\n");
    return;
  }

  // Attempt to receive messages for a short period
  printf("MQTT Waiting to receive messages for 10 seconds...\n");
  std::thread receiveThread(
      [&mqtt]()
      {
        auto receivedMessage = mqtt.receive();
        if (!receivedMessage.second.empty())
          printf("MQTT Received message from Topic \"%s\": %s\n", receivedMessage.first.c_str(), receivedMessage.second.data());
        else
          printf("MQTT No message received\n");
      });
  std::this_thread::sleep_for(std::chrono::seconds(10));

  // Disconnect from AWS MQTT
  mqtt.unsubscribe(evidenceTopic.c_str());
  printf("MQTT Unsubscribed from audio evidence topic\n");
  mqtt.disconnect();
  printf("MQTT Disconnected\n");
  receiveThread.join();
}

int main(void)
{
  // Initialize the AWS SDK and cURL library
  AWS::initialize();
  curl_global_init(CURL_GLOBAL_ALL);

  // Test AWS Secrets Manager
  testSecrets();

  // Test CivicAlert AWS API Endpoint
  testApiEndpoint();

  // Test AWS S3
  AwsSecrets secretManager;
  testS3(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_S3_EVIDENCE_BUCKET).c_str());

  // Test AWS SQS
  testSQS(secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_SQS_DATA_QUEUE_URL).c_str());

  // Test AWS MQTT
  testMQTT();

  // Uninitialize the AWS SDK and cURL library
  curl_global_cleanup();
  AWS::uninitialize();
  return 0;
}

#ifndef __COMMON_HEADER_H__
#define __COMMON_HEADER_H__

#include <cstdint>
#include <string>

// String concatenation helper definition
#define CONCAT_STRINGS(str1, str2) str1 str2

// Make global logger accessible everywhere
class Logger;
extern Logger logger;

// MQTT evidence message constants
#define MQTT_MAX_PAYLOAD_SIZE_BYTES 1016
#define MQTT_MESSAGE_INDEX_MASK 0x7F
#define MQTT_MESSAGE_FINAL_MASK 0x80
typedef struct __attribute__((__packed__))
{
  uint8_t deviceID[7], clipID, messageIdxAndFinal;
  uint8_t data[MQTT_MAX_PAYLOAD_SIZE_BYTES - 9];
} EvidenceMessage;

// Evidence clip Opus framing constants
#define EVIDENCE_OPUS_FRAME_MAX_BYTES 256
typedef struct __attribute__((__packed__, aligned(4)))
{
  uint8_t frameDelimiter, numEncodedBytes;
  uint8_t encodedData[EVIDENCE_OPUS_FRAME_MAX_BYTES];
} EvidenceOpusFrame;

// MQTT alert message constants
typedef struct __attribute__((__packed__))
{
  uint64_t deviceID;
  float lat, lon, height, confidence;
  double timestamp;
  bool usedInFusion;
} AlertMessage;

// Fused incident message
typedef struct
{
  bool fusionSuccessful;
} IncidentMessage;

namespace CivicAlert
{
  // AWS Parameter and Secrets Manager keys
  constexpr const char* AWS_PARAMETER_KEY_MQTT_CREDENTIALS_KEY = "MqttCredentialsKey";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_CA_KEY = "MqttCaKey";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_CLIENT_CERT_KEY = "MqttClientCertKey";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_CLIENT_KEY_KEY = "MqttClientKeyKey";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_DEVICE_INFO_TOPIC = "MqttDeviceInfoTopic";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_ALERTS_TOPIC = "MqttAlertsTopic";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_AUDIO_EVIDENCE_TOPIC = "MqttAudioEvidenceTopic";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_ENDPOINT = "MqttEndpoint";
  constexpr const char* AWS_PARAMETER_KEY_MQTT_PORT = "MqttPort";
  constexpr const char* AWS_PARAMETER_KEY_S3_EVIDENCE_BUCKET = "S3EvidenceBucket";
  constexpr const char* AWS_PARAMETER_KEY_S3_EVIDENCE_URL = "S3EvidenceUrl";
  constexpr const char* AWS_PARAMETER_KEY_SQS_DATA_QUEUE_URL = "SqsDataQueueUrl";
  constexpr const char* AWS_PARAMETER_KEY_SNS_ALERT_TOPIC_ARN = "SnsAlertTopicArn";
  constexpr const char* AWS_PARAMETER_KEY_WEATHER_OPENWEATHERMAP_API_ID = "OpenWeatherMapApiId";
  constexpr const char* AWS_PARAMETER_KEY_WEATHER_TOMORROWIO_API_ID = "TomorrowIoApiId";
  constexpr const char* AWS_PARAMETER_KEY_DB_SECRETS_KEY = "DbSecretsKey";
  constexpr const char* AWS_PARAMETER_KEY_DB_ENDPOINT = "DbEndpoint";
  constexpr const char* AWS_PARAMETER_KEY_DB_PORT = "DbPort";
  constexpr const char* AWS_PARAMETER_KEY_DB_ALERTS_TABLE = "DbAlertsTable";

  // MQTT client settings
  constexpr const char* MQTT_CLOUD_CLIENT_ID = "civicalert-cloud-client";
  constexpr const char* MQTT_EVIDENCE_CLIENT_ID = "civicalert-evidence-client";
  constexpr const uint32_t MQTT_KEEP_ALIVE_SECONDS = 230;

  // Alert database constants
  constexpr const char* DATABASE_NAME = "civicalert";
  constexpr const char* DB_EVIDENCE_CLIP_TABLE_NAME = "evidence_clips";

  // Evidence processing parameters
  constexpr const uint8_t EVIDENCE_OPUS_FRAME_DELIMITER = 0xAA;
  constexpr const char* EVIDENCE_CLIP_FILE_EXTENSION = ".ogg";
  constexpr const int EVIDENCE_AUDIO_SAMPLE_RATE_HZ = 48000;
  constexpr const int EVIDENCE_AUDIO_NUM_CHANNELS = 1;
  constexpr const int EVIDENCE_AUDIO_BITRATE_BPS = 16000;
  constexpr const int EVIDENCE_PROCESSING_TIMEOUT_SECONDS = 3;
  constexpr const unsigned int EVIDENCE_AUDIO_MS_PER_FRAME = 20;
  constexpr const unsigned int EVIDENCE_DECODED_FRAME_SAMPLES = (EVIDENCE_AUDIO_SAMPLE_RATE_HZ / 1000) * EVIDENCE_AUDIO_MS_PER_FRAME;

  // Alert processing parameters
  constexpr const int ALERT_PROCESSING_TIMEOUT_SECONDS = 4;

  // Fusion algorithm parameters
  constexpr const uint8_t FUSION_MIN_NUM_EVENTS = 3;  // TODO: LOWER THIS IF FEWER SENSORS IN AREA, MAYBE MAKE THIS ENTIRELY DEPENDENT ON SENSOR DENSITY
  constexpr const double FUSION_MAX_POSSIBLE_TIME_DIFFERENCE_SECONDS = 2.5;
  constexpr const float FUSION_MAX_POSSIBLE_DISTANCE_METERS = 850.0;

// Process-specific constants
#define LOG_FILE_PATH "/var/log/civicalert/"
  constexpr const char* PROCESS_LOG_PATH = LOG_FILE_PATH;
  constexpr const char* PROCESS_LOG_FILE = CONCAT_STRINGS(LOG_FILE_PATH, "cloud.log");
  constexpr const char* LOG_FILE_ROTATION_DIRECTORY = CONCAT_STRINGS(LOG_FILE_PATH, "to_upload/");
  constexpr const uint32_t LOG_FILE_ROTATION_INTERVAL_SECONDS = 1800;

  // Weather API parameters
  extern std::string WEATHER_API_ID_OPENWEATHERMAP;
  extern std::string WEATHER_API_ID_TOMORROWIO;
}  // namespace CivicAlert

#endif  // #ifndef __COMMON_HEADER_H__

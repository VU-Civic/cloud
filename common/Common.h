#ifndef __COMMON_HEADER_H__
#define __COMMON_HEADER_H__

#include "Logger.h"

// Make global logger accessible everywhere
extern Logger logger;

// MQTT evidence message constants
#define MQTT_MAX_PAYLOAD_SIZE_BYTES 1016
#define MQTT_MESSAGE_INDEX_MASK 0x7F
#define MQTT_MESSAGE_FINAL_MASK 0x80
typedef struct __attribute__((__packed__))
{
  uint32_t deviceID;
  uint8_t messageIdxAndFinal;
  uint8_t data[MQTT_MAX_PAYLOAD_SIZE_BYTES];
} EvidenceMessage;

// Evidence clip Opus framing constants
#define EVIDENCE_OPUS_FRAME_DELIMITER 0xAA
#define EVIDENCE_OPUS_FRAME_MAX_BYTES 256
typedef struct __attribute__((__packed__, aligned(4)))
{
  uint8_t frameDelimiter, numEncodedBytes;
  uint8_t encodedData[EVIDENCE_OPUS_FRAME_MAX_BYTES];
} EvidenceOpusFrame;

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

  // MQTT client settings
  constexpr const char* MQTT_CLOUD_CLIENT_ID = "civicalert-cloud-client";
  constexpr const char* MQTT_EVIDENCE_CLIENT_ID = "civicalert-evidence-client";
  constexpr const uint32_t MQTT_KEEP_ALIVE_SECONDS = 230;

  // Alert database constants
  constexpr const char* ALERT_DATABASE_NAME = "civicalert_alerts_db";  // TODO: STORE IN PARAMETERS
  constexpr const char* ALERT_DATABASE_EVENT_ID_KEY = "event_id";

  // Evidence database constants
  constexpr const char* EVIDENCE_DATABASE_NAME = "civicalert_evidence_db";  // TODO: STORE IN PARAMETERS
  constexpr const char* EVIDENCE_DATABASE_EVENT_ID_KEY = ALERT_DATABASE_EVENT_ID_KEY;
  constexpr const char* EVIDENCE_DATABASE_CLIP_URL_KEY = "evidence_clip_url";

  // Evidence processing parameters
  constexpr const char* EVIDENCE_CLIP_FILE_EXTENSION = ".ogg";
  constexpr const int EVIDENCE_AUDIO_SAMPLE_RATE_HZ = 48000;
  constexpr const int EVIDENCE_AUDIO_NUM_CHANNELS = 1;
  constexpr const int EVIDENCE_AUDIO_BITRATE_BPS = 16000;
  constexpr const unsigned int EVIDENCE_AUDIO_MS_PER_FRAME = 20;
  constexpr const unsigned int EVIDENCE_PROCESSING_TIMEOUT_SECONDS = 3;
  constexpr const unsigned int EVIDENCE_DECODED_FRAME_SAMPLES = (EVIDENCE_AUDIO_SAMPLE_RATE_HZ / 1000) * EVIDENCE_AUDIO_MS_PER_FRAME;

  // Process-specific constants
  constexpr const char* PROCESS_LOG_FILE = "/var/log/civicalert-cloud.log";
  constexpr const Logger::LogLevel LOG_MAX_LEVEL = Logger::INFO;

  // Weather API parameters
  extern std::string WEATHER_API_ID_OPENWEATHERMAP;
  extern std::string WEATHER_API_ID_TOMORROWIO;
  /*

  // Alert message constants
  pub const CELL_IMEI_LENGTH: usize = 15;
  pub const DATA_MAX_TIMESTAMP_IN_PAST_SECONDS: f64 = 60.0;

  // Fusion algorithm parameters
  pub const FUSION_DATA_COLLECTION_SECONDS: u64 = 3;
  pub const FUSION_ALGORITHM_MAX_DISTANCE_METERS: f64 = 700.0;
  pub const FUSION_ALGORITHM_MAX_TIME_DIFF: u64 = 60;
  pub const FUSION_ALGORITHM_MIN_NUM_EVENTS: usize = 3;
  */
}  // namespace CivicAlert

#endif  // #ifndef __COMMON_HEADER_H__

#ifndef __COMMON_HEADER_H__
#define __COMMON_HEADER_H__

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>

// Make global logger accessible everywhere
class Logger;
extern Logger logger;

// String concatenation helper definition
#define CONCAT_STRINGS(str1, str2) str1 str2

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

// Configuration data structure for device settings
typedef struct __attribute__((__packed__))
{
  uint8_t initializedTag;
  uint8_t mqttDeviceInfoQos, mqttAlertQos, mqttAudioQos;
  uint8_t shotDetectionMinThreshold, shotDetectionGoodThreshold;
  uint8_t storageClassificationThreshold, audioClipLengthSeconds;
  uint8_t deviceStatusTransmissionIntervalMinutes;
  uint8_t badAudioRestartAttempted, badAiRestartAttempted;
  uint8_t reserved[21];
} ConfigData;

// Channel alarm bitfield union for MQTT messages
typedef union
{
  uint8_t alarms;
  struct
  {
    uint8_t ch1 : 1;
    uint8_t ch2 : 1;
    uint8_t ch3 : 1;
    uint8_t ch4 : 1;
    uint8_t : 4;
  } alarm;
} ChannelAlarms;

// MQTT device info message constants
#define CELL_IMEI_LENGTH 15
#define CELL_IMSI_LENGTH 15
#define FIRMWARE_VERSION_LENGTH 8
typedef struct __attribute__((__packed__))
{
  uint64_t deviceID;
  char imsi[CELL_IMSI_LENGTH + 1];
  char firmwareVersion[FIRMWARE_VERSION_LENGTH], aiFirmwareVersion[FIRMWARE_VERSION_LENGTH];
  float lat, lon, ht;
  int32_t q1, q2, q3;
  uint8_t signalPower, signalQuality, reserved;
  ChannelAlarms channelAlarms;
  ConfigData deviceConfig;
} DeviceInfoMessage;

// Event information structure for alerts
typedef struct __attribute__((__packed__))
{
  double timestamp;
  float confidence, magnitude, angleOfArrival[3];
} EventInfo;

// MQTT alert message constants
#define MAX_NUM_EVENTS_PER_ALERT 24
typedef struct __attribute__((__packed__))
{
  uint64_t deviceID;
  int32_t sensorQ1, sensorQ2, sensorQ3;
  float sensorLat, sensorLon, sensorHt;
  uint8_t audioLocatorID, numEvents, cellSignalPower, cellSignalQuality;
  EventInfo events[MAX_NUM_EVENTS_PER_ALERT];
} AlertMessage;

// Gunshot report structure for individual extracted shot alerts
typedef struct GunshotReport
{
  GunshotReport(std::chrono::steady_clock::time_point receptionTime, uint64_t deviceID, double timestamp, float lat, float lon, float ht, float x, float y, float z,
                float sourceAzimuth, float sourceElevation, float confidence, float orientationQW, float orientationQX, float orientationQY, float orientationQZ,
                uint8_t audioLocatorID)
      : receptionTime(receptionTime), reportInUse(false), deviceID(deviceID), reportID(0), timestamp(timestamp), lat(lat), lon(lon), ht(ht), x(x), y(y), z(z),
        sourceAzimuth(sourceAzimuth), sourceElevation(sourceElevation), confidence(confidence), orientationQW(orientationQW), orientationQX(orientationQX),
        orientationQY(orientationQY), orientationQZ(orientationQZ), audioLocatorID(audioLocatorID), usedInFusion(false)
  {
  }
  std::chrono::steady_clock::time_point receptionTime;
  std::atomic_flag reportInUse;
  uint64_t deviceID, reportID;
  double timestamp;
  float lat, lon, ht, x, y, z, east, north, up;
  float sourceAzimuth, sourceElevation, confidence;
  float orientationQW, orientationQX, orientationQY, orientationQZ;
  uint8_t audioLocatorID;
  bool usedInFusion;
} GunshotReport;

// Packet structure for reports that are pending fusion
typedef struct
{
  std::shared_ptr<GunshotReport> validationReport;
  std::vector<std::shared_ptr<GunshotReport>> reports;
} ReportPacket;

// Fused incident message
typedef struct
{
  float east, north, up;
  float lat, lon, ht, confidence;
  std::vector<double> eventTimes;
  std::vector<std::shared_ptr<GunshotReport>> contributingReports;
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
  constexpr const char* AWS_PARAMETER_KEY_API_ENDPOINT_TOKEN_KEY = "ApiEndpointTokenKey";
  constexpr const char* AWS_PARAMETER_KEY_API_ENDPOINT_URL = "ApiEndpointUrl";
  constexpr const char* AWS_PARAMETER_KEY_DB_SECRETS_KEY = "DbSecretsKey";
  constexpr const char* AWS_PARAMETER_KEY_DB_ENDPOINT = "DbEndpoint";
  constexpr const char* AWS_PARAMETER_KEY_DB_PORT = "DbPort";

  // MQTT client settings
  constexpr const char* MQTT_CLOUD_CLIENT_ID = "civicalert-cloud-client";
  constexpr const char* MQTT_EVIDENCE_CLIENT_ID = "civicalert-evidence-client";
  constexpr const uint32_t MQTT_KEEP_ALIVE_SECONDS = 230;

  // Alert database constants
  constexpr const char* DATABASE_NAME = "civicalert";
  constexpr const char* DB_EVIDENCE_CLIP_TABLE_NAME = "clips";
  constexpr const char* DB_INCIDENTS_TABLE_NAME = "fused_events";
  constexpr const char* DB_ALERTS_TABLE_NAME = "raw_events";
  constexpr const char* DB_DEVICES_TABLE_NAME = "devices";
  constexpr const char* API_FUSED_EVENT_ENDPOINT = "/events/fused";

  // Evidence processing parameters
  constexpr const uint8_t EVIDENCE_OPUS_FRAME_DELIMITER = 0xAA;
  constexpr const char* EVIDENCE_CLIP_FILE_EXTENSION = ".ogg";
  constexpr const int EVIDENCE_AUDIO_SAMPLE_RATE_HZ = 48000;
  constexpr const int EVIDENCE_AUDIO_NUM_CHANNELS = 1;
  constexpr const int EVIDENCE_AUDIO_BITRATE_BPS = 16000;
  constexpr const int EVIDENCE_PROCESSING_TIMEOUT_SECONDS = 4;
  constexpr const unsigned int EVIDENCE_AUDIO_MS_PER_FRAME = 20;
  constexpr const unsigned int EVIDENCE_DECODED_FRAME_SAMPLES = (EVIDENCE_AUDIO_SAMPLE_RATE_HZ / 1000) * EVIDENCE_AUDIO_MS_PER_FRAME;

  // Alert processing parameters
  constexpr const int ALERT_MIN_PROCESSING_TIMEOUT_MS = 5000;
  constexpr const int ALERT_NON_REPORT_PROCESSING_TIMEOUT_MS = 1000;

  // Fusion algorithm parameters
  constexpr const uint8_t FUSION_MIN_NUM_EVENTS = 3;  // TODO: LOWER THIS IF FEWER SENSORS IN AREA, MAYBE MAKE THIS ENTIRELY DEPENDENT ON SENSOR DENSITY
  constexpr const int FUSION_MAX_NUM_EVENTS = 32;
  constexpr const double FUSION_MAX_POSSIBLE_TIME_DIFFERENCE_SECONDS = 0.8;
  constexpr const float FUSION_MAX_POSSIBLE_DISTANCE_METERS = 1000.0;
  constexpr const int MIN_NUM_HOUGH_PEAKS_FOR_FUSION = 4;
  constexpr const float HOUGH_TRANSFORM_FINE_RESOLUTION_METERS = 1.0f;  // TODO: DO I NEED ALL OF THESE
  constexpr const float HOUGH_TRANSFORM_MEDIUM_RESOLUTION_METERS = 5.0f;
  constexpr const float HOUGH_TRANSFORM_COARSE_RESOLUTION_METERS = 10.0f;

// Process-specific constants
#define LOG_FILE_PATH "/var/log/civicalert/"
  constexpr const char* PROCESS_LOG_PATH = LOG_FILE_PATH;
  constexpr const char* PROCESS_LOG_FILE = CONCAT_STRINGS(LOG_FILE_PATH, "cloud.log");
  constexpr const char* LOG_FILE_ROTATION_DIRECTORY = CONCAT_STRINGS(LOG_FILE_PATH, "to_upload/");
  constexpr const uint32_t LOG_FILE_ROTATION_INTERVAL_SECONDS = 1800;
}  // namespace CivicAlert

constexpr float SPEED_OF_SOUND_IN_AIR_METERS_PER_SECOND(float temperatureInCelsius = 20.0f) { return 20.05f * std::sqrt(temperatureInCelsius + 273.15f); };

#endif  // #ifndef __COMMON_HEADER_H__

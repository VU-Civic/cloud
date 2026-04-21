#include <fstream>
#include "Common.h"
#include "Logger.h"
#include "Transcoding.h"

// Global application logger
Logger logger("/tmp/civicalert.log", Logger::DEBUG);

void test_hex_encoding(const uint8_t* data, uint32_t length)
{
  // Test the hex encoding function with a known input and output
  std::vector<uint8_t> encodedData(2 * length);
  Transcoding::hexEncode(encodedData.data(), data, length);
  std::vector<uint8_t> decodedData = Transcoding::hexDecode(std::move(encodedData));
  if (decodedData != std::vector<uint8_t>(data, data + length))
    logger.log(Logger::ERROR, "Hex encoding/decoding failed: decoded data does not match original\n");
  else
    logger.log(Logger::INFO, "Hex encoding/decoding successful\n");
}

void test_base64_encoding(const uint8_t* data, uint32_t length)
{
  // Test the Base64 encoding function with a known input and output
  std::vector<uint8_t> encodedData(((length + 2) / 3) * 4);
  Transcoding::base64Encode(encodedData.data(), data, length);
  std::vector<uint8_t> decodedData = Transcoding::base64Decode(std::move(encodedData));
  if (decodedData != std::vector<uint8_t>(data, data + length))
    logger.log(Logger::ERROR, "Base64 encoding/decoding failed: decoded data does not match original\n");
  else
    logger.log(Logger::INFO, "Base64 encoding/decoding successful\n");
}

void test_base85_encoding(const uint8_t* data, uint32_t length)
{
  // Test the Base85 encoding function with a known input and output
  std::vector<uint8_t> encodedData(((length + 3) / 4) * 5);
  Transcoding::base85Encode(encodedData.data(), data, length);
  std::vector<uint8_t> decodedData = Transcoding::base85Decode(std::move(encodedData));
  if (decodedData != std::vector<uint8_t>(data, data + length))
    logger.log(Logger::ERROR, "Base85 encoding/decoding failed: decoded data does not match original\n");
  else
    logger.log(Logger::INFO, "Base85 encoding/decoding successful\n");
}

void test_yenc_encoding(const uint8_t* data, uint32_t length)
{
  // Test the yEnc encoding function with a known input and output
  std::vector<uint8_t> encodedData(length * 2);  // Worst-case scenario where every byte needs to be escaped
  Transcoding::yEncEncode(encodedData.data(), data, length);
  std::vector<uint8_t> decodedData = Transcoding::yEncDecode(std::move(encodedData));
  if (decodedData != std::vector<uint8_t>(data, data + length))
    logger.log(Logger::ERROR, "yEnc encoding/decoding failed: decoded data does not match original\n");
  else
    logger.log(Logger::INFO, "yEnc encoding/decoding successful\n");
}

void decode_device_info_message(const std::string& encodedData)
{
  // Decode the hex-encoded device info message and log its contents
  logger.log(Logger::INFO, "Original hex string (length = %zu): %s\n", encodedData.length(), encodedData.c_str());
  std::vector<uint8_t> data(encodedData.begin(), encodedData.end());
  std::vector<uint8_t> decodedData = Transcoding::hexDecode(std::move(data));
  logger.log(Logger::INFO, "Decoded Device Info Message (length = %zu):\n", decodedData.size());
  DeviceInfoMessage* decodedInfo = reinterpret_cast<DeviceInfoMessage*>(decodedData.data());
  logger.log(Logger::INFO, "  deviceID: %llu\n", decodedInfo->deviceID);
  logger.log(Logger::INFO, "  imsi: %s\n", decodedInfo->imsi);
  logger.log(Logger::INFO, "  firmwareVersion: %s\n", decodedInfo->firmwareVersion);
  logger.log(Logger::INFO, "  aiFirmwareVersion: %s\n", decodedInfo->aiFirmwareVersion);
  logger.log(Logger::INFO, "  lat: %f\n", decodedInfo->lat);
  logger.log(Logger::INFO, "  lon: %f\n", decodedInfo->lon);
  logger.log(Logger::INFO, "  ht: %f\n", decodedInfo->ht);
  logger.log(Logger::INFO, "  q1: %d\n", decodedInfo->q1);
  logger.log(Logger::INFO, "  q2: %d\n", decodedInfo->q2);
  logger.log(Logger::INFO, "  q3: %d\n", decodedInfo->q3);
  logger.log(Logger::INFO, "  signalPower: %u\n", decodedInfo->signalPower);
  logger.log(Logger::INFO, "  signalQuality: %u\n", decodedInfo->signalQuality);
  logger.log(Logger::INFO, "  channelAlarms: %u\n", decodedInfo->channelAlarms.alarms);
  logger.log(Logger::INFO, "  deviceConfig.initializedTag: %u\n", decodedInfo->deviceConfig.initializedTag);
  logger.log(Logger::INFO, "  deviceConfig.mqttDeviceInfoQos: %u\n", decodedInfo->deviceConfig.mqttDeviceInfoQos);
  logger.log(Logger::INFO, "  deviceConfig.mqttAlertQos: %u\n", decodedInfo->deviceConfig.mqttAlertQos);
  logger.log(Logger::INFO, "  deviceConfig.mqttAudioQos: %u\n", decodedInfo->deviceConfig.mqttAudioQos);
  logger.log(Logger::INFO, "  deviceConfig.shotDetectionMinThreshold: %u\n", decodedInfo->deviceConfig.shotDetectionMinThreshold);
  logger.log(Logger::INFO, "  deviceConfig.shotDetectionGoodThreshold: %u\n", decodedInfo->deviceConfig.shotDetectionGoodThreshold);
  logger.log(Logger::INFO, "  deviceConfig.storageClassificationThreshold: %u\n", decodedInfo->deviceConfig.storageClassificationThreshold);
  logger.log(Logger::INFO, "  deviceConfig.audioClipLengthSeconds: %u\n", decodedInfo->deviceConfig.audioClipLengthSeconds);
  logger.log(Logger::INFO, "  deviceConfig.deviceStatusTransmissionIntervalMinutes: %u\n", decodedInfo->deviceConfig.deviceStatusTransmissionIntervalMinutes);
  logger.log(Logger::INFO, "  deviceConfig.badAudioRestartAttempted: %u\n", decodedInfo->deviceConfig.badAudioRestartAttempted);
  logger.log(Logger::INFO, "  deviceConfig.badAiRestartAttempted: %u\n", decodedInfo->deviceConfig.badAiRestartAttempted);
}

int main(void)
{
  // Read the binary audio data from assets/gunshot1.wav
  std::vector<uint8_t> audioData;
  {
    std::ifstream inputFile("assets/gunshot1.wav", std::ios::binary);
    if (!inputFile) throw std::runtime_error("Failed to open input file");
    inputFile.seekg(0, std::ios::end);
    const size_t fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);
    audioData.resize(fileSize);
    inputFile.read(reinterpret_cast<char*>(audioData.data()), fileSize);
  }

  // Test the various transcoding implementations
  logger.log(Logger::INFO, "Testing transcoding functions with audio data of size %zu bytes\n", audioData.size());
  test_hex_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));
  test_base64_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));
  test_base85_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));
  test_yenc_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));

  // Test the hex encoding with a known struct input and output
  const DeviceInfoMessage detectionInfo = {.deviceID = 353760970051308,
                                           .imsi = "234500091090845",
                                           .firmwareVersion = "abcdef1",
                                           .aiFirmwareVersion = "0000000",
                                           .lat = 37.7749f,
                                           .lon = -122.4194f,
                                           .ht = 10.0f,
                                           .q1 = 100,
                                           .q2 = 200,
                                           .q3 = 300,
                                           .signalPower = 75,
                                           .signalQuality = 80,
                                           .channelAlarms{.alarms = 0b00001101},
                                           .deviceConfig{.initializedTag = 1,
                                                         .mqttDeviceInfoQos = 1,
                                                         .mqttAlertQos = 2,
                                                         .mqttAudioQos = 0,
                                                         .shotDetectionMinThreshold = 10,
                                                         .shotDetectionGoodThreshold = 20,
                                                         .storageClassificationThreshold = 15,
                                                         .audioClipLengthSeconds = 5,
                                                         .deviceStatusTransmissionIntervalMinutes = 60,
                                                         .badAudioRestartAttempted = 0,
                                                         .badAiRestartAttempted = 0}};
  test_hex_encoding(reinterpret_cast<const uint8_t*>(&detectionInfo), sizeof(DeviceInfoMessage));

  // Test decoding a hex-encoded device info message``
  std::string encodedDeviceInfoMessage(
      "647BF464BE4101003233343530323131303539313536320061626364656631320000000000000000669410426F96ADC2AAE1804328789DFEF67175FDB4F42D01FFFF000055000100323C003C050000FFFFFFFFFFFFFF"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFF");
  decode_device_info_message(encodedDeviceInfoMessage);
  return 0;
}

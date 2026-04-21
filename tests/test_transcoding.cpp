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

void test_hex_encoding2(void)
{
  // Test the hex encoding function with a known input and output
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
  std::vector<uint8_t> encodedData(2 * sizeof(DeviceInfoMessage));
  Transcoding::hexEncode(encodedData.data(), reinterpret_cast<const uint8_t*>(&detectionInfo), sizeof(DeviceInfoMessage));
  std::vector<uint8_t> decodedData = Transcoding::hexDecode(std::move(encodedData));
  if (memcmp(decodedData.data(), &detectionInfo, sizeof(DeviceInfoMessage)) != 0)
    logger.log(Logger::ERROR, "Hex encoding2 failed: decoded data does not match original\n");
  else
    logger.log(Logger::INFO, "Hex encoding2 successful\n");
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
  logger.log(Logger::DEBUG, "Testing transcoding functions with audio data of size %zu bytes\n", audioData.size());
  test_hex_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));
  test_base64_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));
  test_base85_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));
  test_yenc_encoding(audioData.data(), static_cast<uint32_t>(audioData.size()));
  test_hex_encoding2();
  return 0;

  return 0;
}

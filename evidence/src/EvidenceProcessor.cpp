#include <ctime>
#include <filesystem>
#include <thread>
#include <opusenc.h>
#include "Common.h"
#include "AwsServices.h"
#include "EvidenceProcessor.h"

std::atomic<uint32_t> EvidenceProcessor::numActiveThreads{0};
std::string EvidenceProcessor::evidenceClipBaseUrl;

void EvidenceProcessor::initialize(void)
{
  // Retrieve the base evidence clip URL from AWS secrets
  evidenceClipBaseUrl = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_S3_EVIDENCE_URL);
  logger.log(Logger::INFO, "Evidence clip base URL set to: %s\n", evidenceClipBaseUrl.c_str());
}

void EvidenceProcessor::cleanup(void)
{
  // Wait until all active threads have finished processing
  while (numActiveThreads.load(std::memory_order_relaxed) > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Ensure that all temporary evidence files have been deleted
  for (const auto& entry : std::filesystem::directory_iterator("/tmp"))
    if (entry.path().extension() == CivicAlert::EVIDENCE_CLIP_FILE_EXTENSION) std::remove(entry.path().c_str());
}

void EvidenceProcessor::processEvidenceData(std::unordered_map<uint32_t, std::vector<std::vector<uint8_t>>>::node_type&& evidence)
{
  // Retrieve the device ID and associated evidence data
  size_t evidenceDataLength = 0;
  const uint32_t deviceID = evidence.key();
  const auto& evidenceData = evidence.mapped();
  for (const auto& chunk : evidenceData) evidenceDataLength += chunk.size();

  // Flatten the evidence data into a contiguous vector
  std::vector<uint8_t> flattenedEvidence;
  flattenedEvidence.reserve(evidenceDataLength);
  for (const auto& chunk : evidenceData) flattenedEvidence.insert(flattenedEvidence.end(), chunk.begin(), chunk.end());

  // Start a new detached thread to process the evidence data
  numActiveThreads.fetch_add(1, std::memory_order_relaxed);
  std::thread worker(&EvidenceProcessor::processEvidenceWorker, deviceID, std::move(flattenedEvidence));
  worker.detach();
}

void EvidenceProcessor::processEvidenceWorker(uint32_t deviceID, std::vector<uint8_t>&& rawEvidence)
{
  // Open a memory stream for reading the raw evidence data
  EvidenceOpusFrame evidenceFrame;
  int16_t decodedPacket[CivicAlert::EVIDENCE_DECODED_FRAME_SAMPLES];
  FILE* evidenceData = fmemopen(rawEvidence.data(), rawEvidence.size(), "rb");
  if (!evidenceData)
  {
    logger.log(Logger::ERROR, "Failed to open evidence data stream for Device #%lu\n", deviceID);
    numActiveThreads.fetch_sub(1, std::memory_order_relaxed);
    return;
  }

  // Create an Opus decoder for parsing the raw evidence data
  int error = 0;
  OpusDecoder* opusDecoder = opus_decoder_create(CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ, CivicAlert::EVIDENCE_AUDIO_NUM_CHANNELS, &error);
  if (error != OPUS_OK)
  {
    logger.log(Logger::ERROR, "Failed to create an Opus stream decoder: %s\n", opus_strerror(error));
    numActiveThreads.fetch_sub(1, std::memory_order_relaxed);
    fclose(evidenceData);
    return;
  }

  // Create an Ogg Opus encoder for writing the full evidence clip
  const std::string fileName = std::to_string(deviceID) + std::string("_") + std::to_string(time(nullptr)) + CivicAlert::EVIDENCE_CLIP_FILE_EXTENSION;
  const std::string localFileName = std::string("/tmp/") + fileName;
  OggOpusComments* comments = ope_comments_create();
  OggOpusEnc* oggEncoder = ope_encoder_create_file(localFileName.c_str(), comments, CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ, CivicAlert::EVIDENCE_AUDIO_NUM_CHANNELS, 0, &error);
  if (comments && oggEncoder)
  {
    ope_comments_add(comments, "ARTIST", "CivicAlert");
    ope_comments_add(comments, "TITLE", "Evidence Clip");
  }
  else
  {
    logger.log(Logger::ERROR, "Failed to create an Ogg Opus encoder: %s\n", opus_strerror(error));
    numActiveThreads.fetch_sub(1, std::memory_order_relaxed);
    if (oggEncoder) ope_encoder_destroy(oggEncoder);
    if (comments) ope_comments_destroy(comments);
    fclose(evidenceData);
    return;
  }
  ope_encoder_ctl(oggEncoder, OPUS_SET_APPLICATION_REQUEST, OPUS_APPLICATION_RESTRICTED_LOWDELAY);
  ope_encoder_ctl(oggEncoder, OPUS_SET_BITRATE_REQUEST, CivicAlert::EVIDENCE_AUDIO_BITRATE_BPS);
  ope_encoder_ctl(oggEncoder, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_MUSIC);
  ope_encoder_ctl(oggEncoder, OPUS_SET_COMPLEXITY_REQUEST, 0);
  ope_encoder_ctl(oggEncoder, OPUS_SET_VBR_REQUEST, 1);

  // Process the audio data in frames
  constexpr const size_t frameHeaderSize = offsetof(EvidenceOpusFrame, encodedData);
  size_t samplesRead = fread(&evidenceFrame, 1, frameHeaderSize, evidenceData);
  while ((samplesRead == frameHeaderSize) && (evidenceFrame.frameDelimiter == CivicAlert::EVIDENCE_OPUS_FRAME_DELIMITER))
  {
    // Read the next audio frame
    samplesRead = fread(&evidenceFrame.encodedData, 1, evidenceFrame.numEncodedBytes, evidenceData);
    if (samplesRead != evidenceFrame.numEncodedBytes) break;
    logger.log(Logger::DEBUG, "Processing data frame of size %zu for Device #%lu\n", samplesRead, deviceID);

    // Decode the audio frame and re-encode it as Ogg in the output file
    const unsigned char* dataPtr = evidenceFrame.numEncodedBytes ? evidenceFrame.encodedData : nullptr;
    int numDecodedSamples = opus_decode(opusDecoder, dataPtr, evidenceFrame.numEncodedBytes, decodedPacket, CivicAlert::EVIDENCE_DECODED_FRAME_SAMPLES, 0);
    ope_encoder_write(oggEncoder, decodedPacket, numDecodedSamples);

    // Read the next frame header
    samplesRead = fread(&evidenceFrame, 1, frameHeaderSize, evidenceData);
  }

  // Finalize Ogg Opus encoding and clean up the resources
  ope_encoder_drain(oggEncoder);
  ope_encoder_destroy(oggEncoder);
  ope_comments_destroy(comments);
  fclose(evidenceData);

  // Store the evidence clip into S3 and update the database record
  const std::string evidenceClipUrl = evidenceClipBaseUrl + fileName;
  logger.log(Logger::INFO, "Storing evidence clip to AWS S3: %s\n", evidenceClipUrl.c_str());
  if (AwsServices::storeEvidenceClipToS3(fileName.c_str(), localFileName.c_str()))
  {
    // TODO: AwsServices::updateEvidenceDatabaseRecord(deviceID, evidenceClipUrl);
  }
  else
    logger.log(Logger::ERROR, "Failed to store clip!\n");

  // Delete the temporary local evidence clip file and decrement the active thread count
  std::remove(localFileName.c_str());
  numActiveThreads.fetch_sub(1, std::memory_order_relaxed);
}

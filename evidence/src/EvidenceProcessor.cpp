#include <ctime>
#include <filesystem>
#include <thread>
#include <opus/opusenc.h>
#include "Common.h"
#include "AwsServices.h"
#include "EvidenceProcessor.h"

int EvidenceProcessor::referenceCount = 0;
std::mutex EvidenceProcessor::initializationMutex;
std::atomic_uint32_t EvidenceProcessor::numActiveThreads{0};
std::unique_ptr<PostgreSQL> EvidenceProcessor::evidenceDatabase;

void EvidenceProcessor::connectToEvidenceDatabase(void)
{
  // Retrieve the database endpoint, username, and password from AWS secrets
  const auto dbEndpoint = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_DB_ENDPOINT);
  const auto dbSecretsKey = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_DB_SECRETS_KEY);
  const auto dbPort = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_DB_PORT);
  const auto dbSecrets = AwsServices::getSecret(dbSecretsKey.c_str());
  const auto dbUsername = AwsServices::extractSecretValue(dbSecrets, "username");
  const auto dbPassword = AwsServices::extractSecretValue(dbSecrets, "password");
  if (dbEndpoint.empty() || dbPort.empty() || dbSecretsKey.empty() || dbUsername.empty() || dbPassword.empty())
  {
    logger.log(Logger::ERROR, "Database endpoint, port, or secrets parameter is empty...restarting process!\n");
    exit(EXIT_FAILURE);
  }

  // Create the database client and attempt to connect
  evidenceDatabase = std::make_unique<PostgreSQL>(dbEndpoint.c_str(), dbPort.c_str(), CivicAlert::DATABASE_NAME, dbUsername.c_str(), dbPassword.c_str());
  if (!evidenceDatabase->connect())
  {
    logger.log(Logger::ERROR, "Failed to connect to the evidence database...restarting process!\n");
    exit(EXIT_FAILURE);
  }
  evidenceDatabase->executeQuery("SET search_path TO public;");
}

void EvidenceProcessor::initialize(void)
{
  // Only allow one complete initialization
  std::lock_guard<std::mutex> lock(initializationMutex);
  if (referenceCount++ > 0) return;

  // Establish a connection to the evidence database
  connectToEvidenceDatabase();
}

void EvidenceProcessor::cleanup(void)
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

  // Wait until all active threads have finished processing
  logger.log(Logger::INFO, "Waiting for all active evidence processing threads to finish...\n");
  while (numActiveThreads.load() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Clean up the database connection
  if (evidenceDatabase)
  {
    evidenceDatabase->disconnect();
    evidenceDatabase.reset();
  }

  // Ensure that all temporary evidence files have been deleted
  logger.log(Logger::INFO, "Cleaning up temporary files...\n");
  std::vector<std::filesystem::path> temporaryFiles;
  for (const auto& entry : std::filesystem::directory_iterator("/tmp"))
    if (entry.path().extension() == CivicAlert::EVIDENCE_CLIP_FILE_EXTENSION) temporaryFiles.push_back(entry.path());
  for (const auto& file : temporaryFiles) std::filesystem::remove(file);
}

void EvidenceProcessor::processEvidenceData(uint64_t deviceID, uint8_t clipID, std::unordered_map<uint64_t, std::vector<std::vector<uint8_t>>>::node_type&& evidence)
{
  // Compute the total byte length of the evidence data
  size_t evidenceDataLength = 0;
  const auto& evidenceData = evidence.mapped();
  for (const auto& chunk : evidenceData) evidenceDataLength += chunk.size();

  // Flatten the evidence data into a contiguous vector
  std::vector<uint8_t> flattenedEvidence;
  flattenedEvidence.reserve(evidenceDataLength);
  for (const auto& chunk : evidenceData) flattenedEvidence.insert(flattenedEvidence.end(), chunk.begin(), chunk.end());

  // Start a new detached thread to process the evidence data
  numActiveThreads.fetch_add(1);
  std::thread worker(&EvidenceProcessor::processEvidenceWorker, deviceID, clipID, std::move(flattenedEvidence));
  worker.detach();
}

void EvidenceProcessor::processEvidenceWorker(uint64_t deviceID, uint8_t clipID, std::vector<uint8_t>&& rawEvidence)
{
  // Open a memory stream for reading the raw evidence data
  EvidenceOpusFrame evidenceFrame;
  int16_t decodedPacket[CivicAlert::EVIDENCE_DECODED_FRAME_SAMPLES];
  auto* evidenceData = fmemopen(rawEvidence.data(), rawEvidence.size(), "rb");
  if (!evidenceData)
  {
    logger.log(Logger::ERROR, "Failed to open evidence data stream for Device #%llu, Clip #%u\n", deviceID, clipID);
    numActiveThreads.fetch_sub(1);
    return;
  }

  // Create an Opus decoder for parsing the raw evidence data
  int error = 0;
  OpusDecoder* opusDecoder = opus_decoder_create(CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ, CivicAlert::EVIDENCE_AUDIO_NUM_CHANNELS, &error);
  if (error != OPUS_OK)
  {
    logger.log(Logger::ERROR, "Failed to create an Opus stream decoder: %s\n", opus_strerror(error));
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }

  // Create an Ogg Opus encoder for writing the full evidence clip
  const auto fileName = std::to_string(deviceID) + "_" + std::to_string(time(nullptr)) + CivicAlert::EVIDENCE_CLIP_FILE_EXTENSION;
  const auto localFileName = "/tmp/" + fileName;
  auto* comments = ope_comments_create();
  auto* oggEncoder = ope_encoder_create_file(localFileName.c_str(), comments, CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ, CivicAlert::EVIDENCE_AUDIO_NUM_CHANNELS, 0, &error);
  if (comments && oggEncoder)
  {
    ope_comments_add(comments, "ARTIST", "CivicAlert");
    ope_comments_add(comments, "TITLE", "Evidence Clip");
  }
  else
  {
    logger.log(Logger::ERROR, "Failed to create an Ogg Opus encoder: %s\n", opus_strerror(error));
    numActiveThreads.fetch_sub(1);
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
  auto samplesRead = fread(&evidenceFrame, 1, frameHeaderSize, evidenceData);
  while ((samplesRead == frameHeaderSize) && (evidenceFrame.frameDelimiter == CivicAlert::EVIDENCE_OPUS_FRAME_DELIMITER))
  {
    // Read the next audio frame
    samplesRead = fread(&evidenceFrame.encodedData, 1, evidenceFrame.numEncodedBytes, evidenceData);
    if (samplesRead != evidenceFrame.numEncodedBytes) break;
    logger.log(Logger::DEBUG, "Processing data frame of size %zu for Device #%llu\n", samplesRead, deviceID);

    // Decode the audio frame and re-encode it as Ogg in the output file
    const unsigned char* dataPtr = evidenceFrame.numEncodedBytes ? evidenceFrame.encodedData : nullptr;
    auto numDecodedSamples = opus_decode(opusDecoder, dataPtr, evidenceFrame.numEncodedBytes, decodedPacket, CivicAlert::EVIDENCE_DECODED_FRAME_SAMPLES, 0);
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
  logger.log(Logger::INFO, "Storing evidence clip to AWS S3 as %s\n", fileName.c_str());
  if (AwsServices::storeEvidenceClipToS3(fileName.c_str(), localFileName.c_str()))
  {
    const auto evidenceUpdateQuery = std::string("CALL insert_or_update_clip(") + std::to_string(deviceID) + std::string("::int8, ") + std::to_string(clipID) +
                                     std::string("::int2, '") + fileName + std::string("'::varchar);");
    while (!evidenceDatabase->executeQuery(evidenceUpdateQuery.c_str()) && !evidenceDatabase->isConnected())
    {
      // Attempt to reestablish a lost database connection
      logger.log(Logger::ERROR, "Failed to update evidence database record for Device #%lu, Clip #%u\n", deviceID, clipID);
      connectToEvidenceDatabase();
    }
  }
  else
    logger.log(Logger::ERROR, "Failed to store clip!\n");

  // Delete the temporary local evidence clip file and decrement the active thread count
  std::filesystem::remove(localFileName);
  numActiveThreads.fetch_sub(1);
}

#include <ctime>
#include <filesystem>
#include <thread>
#include <opus/opus.h>
#include "Common.h"
#include "AwsServices.h"
#include "EvidenceProcessor.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
}

int EvidenceProcessor::referenceCount = 0;
std::atomic_uint32_t EvidenceProcessor::numActiveThreads{0};
std::mutex EvidenceProcessor::initializationMutex, EvidenceProcessor::dbMutex;
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

  // Create an M4A (AAC in MP4 container) encoder for writing the full evidence clip
  AVFormatContext* formatCtx = nullptr;
  const auto fileName = std::to_string(deviceID) + "_" + std::to_string(time(nullptr)) + CivicAlert::EVIDENCE_CLIP_FILE_EXTENSION;
  const auto localFileName = "/tmp/" + fileName;
  if (avformat_alloc_output_context2(&formatCtx, nullptr, "mp4", localFileName.c_str()) < 0 || !formatCtx)
  {
    logger.log(Logger::ERROR, "Failed to allocate M4A output context for Device #%llu, Clip #%u\n", deviceID, clipID);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  const AVCodec* aacCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (!aacCodec)
  {
    logger.log(Logger::ERROR, "AAC encoder not found\n");
    avformat_free_context(formatCtx);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  AVCodecContext* codecCtx = avcodec_alloc_context3(aacCodec);
  if (!codecCtx)
  {
    logger.log(Logger::ERROR, "Failed to allocate AAC codec context\n");
    avformat_free_context(formatCtx);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  codecCtx->sample_rate = CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ;
  codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
  codecCtx->bit_rate = CivicAlert::EVIDENCE_AUDIO_BITRATE_BPS;
  codecCtx->ch_layout = AV_CHANNEL_LAYOUT_MONO;
  if (avcodec_open2(codecCtx, aacCodec, nullptr) < 0)
  {
    logger.log(Logger::ERROR, "Failed to open AAC encoder\n");
    avcodec_free_context(&codecCtx);
    avformat_free_context(formatCtx);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  AVStream* stream = avformat_new_stream(formatCtx, nullptr);
  if (!stream)
  {
    logger.log(Logger::ERROR, "Failed to create audio stream\n");
    avcodec_free_context(&codecCtx);
    avformat_free_context(formatCtx);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  avcodec_parameters_from_context(stream->codecpar, codecCtx);
  stream->time_base = {1, CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ};
  if (avio_open(&formatCtx->pb, localFileName.c_str(), AVIO_FLAG_WRITE) < 0 || avformat_write_header(formatCtx, nullptr) < 0)
  {
    logger.log(Logger::ERROR, "Failed to open output file or write M4A header for Device #%llu\n", deviceID);
    avio_closep(&formatCtx->pb);
    avcodec_free_context(&codecCtx);
    avformat_free_context(formatCtx);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  AVFrame* aacFrame = av_frame_alloc();
  AVPacket* aacPacket = av_packet_alloc();
  if (!aacFrame || !aacPacket)
  {
    logger.log(Logger::ERROR, "Failed to allocate AAC frame or packet\n");
    if (aacFrame) av_frame_free(&aacFrame);
    if (aacPacket) av_packet_free(&aacPacket);
    avio_closep(&formatCtx->pb);
    avcodec_free_context(&codecCtx);
    avformat_free_context(formatCtx);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  aacFrame->nb_samples = codecCtx->frame_size;
  aacFrame->format = codecCtx->sample_fmt;
  av_channel_layout_copy(&aacFrame->ch_layout, &codecCtx->ch_layout);
  if (av_frame_get_buffer(aacFrame, 0) < 0)
  {
    logger.log(Logger::ERROR, "Failed to allocate AAC frame buffer\n");
    av_frame_free(&aacFrame);
    av_packet_free(&aacPacket);
    avio_closep(&formatCtx->pb);
    avcodec_free_context(&codecCtx);
    avformat_free_context(formatCtx);
    opus_decoder_destroy(opusDecoder);
    numActiveThreads.fetch_sub(1);
    fclose(evidenceData);
    return;
  }
  int64_t pts = 0;

  // Accumulate decoded S16 samples converted to F32, drained in AAC frame_size (1024) chunks
  size_t pcmReadHead = 0;
  std::vector<float> pcmBuffer;
  pcmBuffer.reserve(codecCtx->frame_size * 4);

  // Encodes one full AAC frame starting at pcmReadHead; advances pcmReadHead by samplesToWrite
  auto encodeFrame = [&](int samplesToWrite)
  {
    av_frame_make_writable(aacFrame);
    auto* dst = reinterpret_cast<float*>(aacFrame->data[0]);
    std::copy(pcmBuffer.begin() + static_cast<ptrdiff_t>(pcmReadHead),
              pcmBuffer.begin() + static_cast<ptrdiff_t>(pcmReadHead) + samplesToWrite, dst);
    if (samplesToWrite < codecCtx->frame_size) std::fill(dst + samplesToWrite, dst + codecCtx->frame_size, 0.0f);
    pcmReadHead += samplesToWrite;
    aacFrame->pts = pts;
    pts += codecCtx->frame_size;
    if (avcodec_send_frame(codecCtx, aacFrame) == 0)
      while (avcodec_receive_packet(codecCtx, aacPacket) == 0)
      {
        av_packet_rescale_ts(aacPacket, codecCtx->time_base, stream->time_base);
        aacPacket->stream_index = stream->index;
        av_interleaved_write_frame(formatCtx, aacPacket);
        av_packet_unref(aacPacket);
      }
  };

  // Process the audio data in frames
  constexpr const size_t frameHeaderSize = offsetof(EvidenceOpusFrame, encodedData);
  auto samplesRead = fread(&evidenceFrame, 1, frameHeaderSize, evidenceData);
  while ((samplesRead == frameHeaderSize) && (evidenceFrame.frameDelimiter == CivicAlert::EVIDENCE_OPUS_FRAME_DELIMITER))
  {
    // Read the next audio frame
    samplesRead = fread(&evidenceFrame.encodedData, 1, evidenceFrame.numEncodedBytes, evidenceData);
    if (samplesRead != evidenceFrame.numEncodedBytes) break;
    logger.log(Logger::DEBUG, "Processing data frame of size %zu for Device #%llu\n", samplesRead, deviceID);

    // Decode the Opus frame to S16 PCM and convert to F32, accumulating in the buffer
    const unsigned char* dataPtr = evidenceFrame.numEncodedBytes ? evidenceFrame.encodedData : nullptr;
    auto numDecodedSamples = opus_decode(opusDecoder, dataPtr, evidenceFrame.numEncodedBytes, decodedPacket, CivicAlert::EVIDENCE_DECODED_FRAME_SAMPLES, 0);
    for (int i = 0; i < numDecodedSamples; ++i) pcmBuffer.push_back(decodedPacket[i] / 32768.0f);

    // Drain full AAC frames from the buffer, then compact the consumed entries
    while (static_cast<int>(pcmBuffer.size() - pcmReadHead) >= codecCtx->frame_size) encodeFrame(codecCtx->frame_size);
    pcmBuffer.erase(pcmBuffer.begin(), pcmBuffer.begin() + static_cast<ptrdiff_t>(pcmReadHead));
    pcmReadHead = 0;

    // Read the next frame header
    samplesRead = fread(&evidenceFrame, 1, frameHeaderSize, evidenceData);
  }

  // Encode any remaining samples (zero-padded to a full AAC frame) then flush the encoder
  if (!pcmBuffer.empty()) encodeFrame(static_cast<int>(pcmBuffer.size()));
  avcodec_send_frame(codecCtx, nullptr);
  while (avcodec_receive_packet(codecCtx, aacPacket) == 0)
  {
    av_packet_rescale_ts(aacPacket, codecCtx->time_base, stream->time_base);
    aacPacket->stream_index = stream->index;
    av_interleaved_write_frame(formatCtx, aacPacket);
    av_packet_unref(aacPacket);
  }

  // Finalize M4A encoding and clean up resources
  av_write_trailer(formatCtx);
  av_frame_free(&aacFrame);
  av_packet_free(&aacPacket);
  avcodec_free_context(&codecCtx);
  avio_closep(&formatCtx->pb);
  avformat_free_context(formatCtx);
  opus_decoder_destroy(opusDecoder);
  fclose(evidenceData);

  // Store the evidence clip into S3 and update the database record
  logger.log(Logger::INFO, "Storing evidence clip to AWS S3 as %s\n", fileName.c_str());
  if (AwsServices::storeEvidenceClipToS3(fileName.c_str(), localFileName.c_str()))
  {
    std::lock_guard<std::mutex> dbLock(dbMutex);
    const auto evidenceUpdateQuery = std::string("CALL insert_or_update_clip(") + std::to_string(deviceID) + std::string("::int8, ") + std::to_string(clipID) +
                                     std::string("::int2, '") + fileName + std::string("'::varchar);");
    while (!evidenceDatabase->executeQuery(evidenceUpdateQuery.c_str()) && !evidenceDatabase->isConnected())
    {
      // Attempt to reestablish a lost database connection
      logger.log(Logger::ERROR, "Failed to update evidence database record for Device #%llu, Clip #%u\n", deviceID, clipID);
      connectToEvidenceDatabase();
    }
  }
  else
    logger.log(Logger::ERROR, "Failed to store clip!\n");

  // Delete the temporary local evidence clip file and decrement the active thread count
  std::filesystem::remove(localFileName);
  numActiveThreads.fetch_sub(1);
}

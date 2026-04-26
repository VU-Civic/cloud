#include <ctime>
#include <filesystem>
#include <opus/opus.h>
#include <unistd.h>
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
std::atomic_bool EvidenceProcessor::workersShouldStop{false};
std::mutex EvidenceProcessor::initializationMutex, EvidenceProcessor::dbMutex, EvidenceProcessor::workQueueMutex;
std::condition_variable EvidenceProcessor::workQueueCV;
std::vector<std::thread> EvidenceProcessor::workerThreads;
std::queue<EvidenceProcessor::WorkItem> EvidenceProcessor::workQueue;
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

  // Start the worker thread pool with one thread per CPU core
  const size_t num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
  workersShouldStop.store(false);
  workerThreads.reserve(num_cpus);
  for (size_t i = 0; i < num_cpus; ++i) workerThreads.emplace_back(&EvidenceProcessor::workerThread);
}

void EvidenceProcessor::cleanup(void)
{
  // Only allow one complete cleanup
  {
    std::lock_guard<std::mutex> lock(initializationMutex);
    if (--referenceCount > 0) return;
    if (referenceCount < 0)
    {
      // Disallow unmatched uninitializations
      referenceCount = 0;
      return;
    }
  }

  // Signal workers to stop and wait for them to finish processing any active clips
  logger.log(Logger::INFO, "Waiting for all active evidence processing threads to finish...\n");
  {
    std::lock_guard<std::mutex> lock(workQueueMutex);
    workersShouldStop.store(true);
  }
  workQueueCV.notify_all();
  for (auto& t : workerThreads)
    if (t.joinable()) t.join();
  workerThreads.clear();

  // Clean up the database connection
  {
    std::lock_guard<std::mutex> dbLock(dbMutex);
    if (evidenceDatabase)
    {
      evidenceDatabase->disconnect();
      evidenceDatabase.reset();
    }
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

  // Enqueue the work item for a worker thread to process
  {
    std::lock_guard<std::mutex> lock(workQueueMutex);
    workQueue.push({deviceID, clipID, std::move(flattenedEvidence)});
  }
  workQueueCV.notify_one();
}

void EvidenceProcessor::workerThread(void)
{
  // Initialize an Opus decoder
  int error = 0;
  OpusDecoder* opusDecoder = opus_decoder_create(CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ, CivicAlert::EVIDENCE_AUDIO_NUM_CHANNELS, &error);
  if (!opusDecoder || error != OPUS_OK)
  {
    logger.log(Logger::ERROR, "Worker thread failed to create Opus decoder: %s\n", opus_strerror(error));
    return;
  }

  // Initialize an AAC codec context
  static const AVCodec* aacCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (!aacCodec)
  {
    logger.log(Logger::ERROR, "Worker thread: AAC encoder not found\n");
    opus_decoder_destroy(opusDecoder);
    return;
  }
  AVCodecContext* codecCtx = avcodec_alloc_context3(aacCodec);
  if (!codecCtx)
  {
    logger.log(Logger::ERROR, "Worker thread failed to allocate AAC codec context\n");
    opus_decoder_destroy(opusDecoder);
    return;
  }
  codecCtx->sample_rate = CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ;
  codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
  codecCtx->bit_rate = CivicAlert::EVIDENCE_AUDIO_BITRATE_BPS;
  codecCtx->ch_layout = AV_CHANNEL_LAYOUT_MONO;
  if (avcodec_open2(codecCtx, aacCodec, nullptr) < 0)
  {
    logger.log(Logger::ERROR, "Worker thread failed to open AAC encoder\n");
    avcodec_free_context(&codecCtx);
    opus_decoder_destroy(opusDecoder);
    return;
  }

  // Pre-allocate an AVFrame and AVPacket once
  AVFrame* aacFrame = av_frame_alloc();
  AVPacket* aacPacket = av_packet_alloc();
  if (!aacFrame || !aacPacket)
  {
    logger.log(Logger::ERROR, "Worker thread failed to allocate AAC frame or packet\n");
    if (aacFrame) av_frame_free(&aacFrame);
    if (aacPacket) av_packet_free(&aacPacket);
    avcodec_free_context(&codecCtx);
    opus_decoder_destroy(opusDecoder);
    return;
  }
  aacFrame->nb_samples = codecCtx->frame_size;
  aacFrame->format = codecCtx->sample_fmt;
  av_channel_layout_copy(&aacFrame->ch_layout, &codecCtx->ch_layout);
  if (av_frame_get_buffer(aacFrame, 0) < 0)
  {
    logger.log(Logger::ERROR, "Worker thread failed to allocate AAC frame buffer\n");
    av_frame_free(&aacFrame);
    av_packet_free(&aacPacket);
    avcodec_free_context(&codecCtx);
    opus_decoder_destroy(opusDecoder);
    return;
  }

  // Create a PCM conversion buffer
  std::vector<float> pcmBuffer;
  pcmBuffer.reserve(codecCtx->frame_size * 4);

  // Process clips from the work queue until shutdown
  while (true)
  {
    WorkItem item;
    {
      std::unique_lock<std::mutex> lock(workQueueMutex);
      workQueueCV.wait(lock, [] { return !workQueue.empty() || workersShouldStop.load(); });
      if (workQueue.empty()) break;
      item = std::move(workQueue.front());
      workQueue.pop();
    }
    processClip(item.deviceID, item.clipID, item.rawEvidence, opusDecoder, codecCtx, aacFrame, aacPacket, pcmBuffer);

    // Reset the codec state for the next clip
    avcodec_flush_buffers(codecCtx);
    opus_decoder_ctl(opusDecoder, OPUS_RESET_STATE);
    pcmBuffer.clear();
  }

  // Release per-thread resources on exit
  av_frame_free(&aacFrame);
  av_packet_free(&aacPacket);
  avcodec_free_context(&codecCtx);
  opus_decoder_destroy(opusDecoder);
}

void EvidenceProcessor::processClip(uint64_t deviceID, uint8_t clipID, const std::vector<uint8_t>& rawEvidence, OpusDecoder* opusDecoder, AVCodecContext* codecCtx,
                                    AVFrame* aacFrame, AVPacket* aacPacket, std::vector<float>& pcmBuffer)
{
  // Open a memory stream for reading the raw evidence data
  EvidenceOpusFrame evidenceFrame;
  int16_t decodedPacket[CivicAlert::EVIDENCE_DECODED_FRAME_SAMPLES];
  auto* evidenceData = fmemopen(const_cast<uint8_t*>(rawEvidence.data()), rawEvidence.size(), "rb");
  if (!evidenceData)
  {
    logger.log(Logger::ERROR, "Failed to open evidence data stream for Device #%llu, Clip #%u\n", deviceID, clipID);
    return;
  }

  // Create an M4A (AAC in MP4 container) output file for this clip
  AVFormatContext* formatCtx = nullptr;
  const auto fileName = std::to_string(deviceID) + "_" + std::to_string(time(nullptr)) + CivicAlert::EVIDENCE_CLIP_FILE_EXTENSION;
  const auto localFileName = "/tmp/" + fileName;
  if (avformat_alloc_output_context2(&formatCtx, nullptr, "mp4", localFileName.c_str()) < 0 || !formatCtx)
  {
    logger.log(Logger::ERROR, "Failed to allocate M4A output context for Device #%llu, Clip #%u\n", deviceID, clipID);
    fclose(evidenceData);
    return;
  }
  AVStream* stream = avformat_new_stream(formatCtx, nullptr);
  if (!stream)
  {
    logger.log(Logger::ERROR, "Failed to create audio stream for Device #%llu, Clip #%u\n", deviceID, clipID);
    avformat_free_context(formatCtx);
    fclose(evidenceData);
    return;
  }
  avcodec_parameters_from_context(stream->codecpar, codecCtx);
  stream->time_base = {1, CivicAlert::EVIDENCE_AUDIO_SAMPLE_RATE_HZ};
  if (avio_open(&formatCtx->pb, localFileName.c_str(), AVIO_FLAG_WRITE) < 0 || avformat_write_header(formatCtx, nullptr) < 0)
  {
    logger.log(Logger::ERROR, "Failed to open output file or write M4A header for Device #%llu\n", deviceID);
    avio_closep(&formatCtx->pb);
    avformat_free_context(formatCtx);
    fclose(evidenceData);
    return;
  }

  // Encodes one AAC frame's worth of samples from pcmBuffer starting at pcmReadHead
  int64_t pts = 0;
  size_t pcmReadHead = 0;
  auto encodeFrame = [&](int samplesToWrite)
  {
    av_frame_make_writable(aacFrame);
    auto* dst = reinterpret_cast<float*>(aacFrame->data[0]);
    std::copy(pcmBuffer.begin() + static_cast<ptrdiff_t>(pcmReadHead), pcmBuffer.begin() + static_cast<ptrdiff_t>(pcmReadHead) + samplesToWrite, dst);
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

  // Finalize the M4A file and release per-clip resources
  av_write_trailer(formatCtx);
  avio_closep(&formatCtx->pb);
  avformat_free_context(formatCtx);
  fclose(evidenceData);

  // Store the evidence clip into S3 and update the database record
  logger.log(Logger::INFO, "Storing evidence clip to AWS S3 as %s\n", fileName.c_str());
  if (AwsServices::storeEvidenceClipToS3(fileName.c_str(), localFileName.c_str()))
  {
    std::lock_guard<std::mutex> dbLock(dbMutex);
    const auto evidenceUpdateQuery = std::string("CALL insert_or_update_clip(") + std::to_string(deviceID) + std::string("::int8, ") + std::to_string(clipID) +
                                     std::string("::int2, '") + fileName + std::string("'::varchar);");
    if (!evidenceDatabase->executeQuery(evidenceUpdateQuery.c_str()))
    {
      logger.log(Logger::ERROR, "Failed to update evidence database record for Device #%llu, Clip #%u\n", deviceID, clipID);
      if (!evidenceDatabase->isConnected())
      {
        connectToEvidenceDatabase();
        evidenceDatabase->executeQuery(evidenceUpdateQuery.c_str());
      }
    }
  }
  else
    logger.log(Logger::ERROR, "Failed to store clip!\n");

  // Delete the temporary local evidence clip file
  std::filesystem::remove(localFileName);
}

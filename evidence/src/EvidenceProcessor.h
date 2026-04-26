#ifndef __EVIDENCE_PROCESSOR_HEADER_H__
#define __EVIDENCE_PROCESSOR_HEADER_H__

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>
#include "PostgreSQL.h"

// Forward declarations for opaque codec types
typedef struct AVCodecContext AVCodecContext;
typedef struct AVFrame AVFrame;
typedef struct AVPacket AVPacket;
typedef struct OpusDecoder OpusDecoder;

class EvidenceProcessor final
{
public:

  // Initialization/cleanup
  static void initialize(void);
  static void cleanup(void);

  // Evidence processing functions
  static void processEvidenceData(uint64_t deviceID, uint8_t clipID, std::unordered_map<uint64_t, std::vector<std::vector<uint8_t>>>::node_type&& evidence);

private:

  // Work item passed from the enqueue side to a worker thread
  struct WorkItem
  {
    uint64_t deviceID;
    uint8_t clipID;
    std::vector<uint8_t> rawEvidence;
  };

  // Helper function to connect to the evidence database
  static void connectToEvidenceDatabase(void);

  // Thread pool worker that owns per-thread codec resources and processes clips from the work queue
  static void workerThread(void);

  // Per-clip processing logic
  static void processClip(uint64_t deviceID, uint8_t clipID, const std::vector<uint8_t>& rawEvidence, OpusDecoder* opusDecoder, AVCodecContext* codecCtx, AVFrame* aacFrame,
                          AVPacket* aacPacket, std::vector<float>& pcmBuffer);

  // Private member variables
  static int referenceCount;
  static std::atomic_bool workersShouldStop;
  static std::mutex initializationMutex, dbMutex, workQueueMutex;
  static std::condition_variable workQueueCV;
  static std::vector<std::thread> workerThreads;
  static std::queue<WorkItem> workQueue;
  static std::unique_ptr<PostgreSQL> evidenceDatabase;
};

#endif  // #ifndef __EVIDENCE_PROCESSOR_HEADER_H__

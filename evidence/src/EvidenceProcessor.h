#ifndef __EVIDENCE_PROCESSOR_HEADER_H__
#define __EVIDENCE_PROCESSOR_HEADER_H__

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "PostgreSQL.h"

class EvidenceProcessor final
{
public:

  // Initialization/cleanup
  static void initialize(void);
  static void cleanup(void);

  // Evidence processing functions
  static void processEvidenceData(uint32_t deviceID, uint8_t clipID, std::unordered_map<uint64_t, std::vector<std::vector<uint8_t>>>::node_type&& evidence);

private:

  // Helper function to connect to the evidence database
  static void connectToEvidenceDatabase(void);

  // Thread worker functions
  static void processEvidenceWorker(uint32_t deviceID, uint8_t clipID, std::vector<uint8_t>&& rawEvidence);

  // Private member variables
  static int referenceCount;
  static std::mutex initializationMutex;
  static std::atomic<uint32_t> numActiveThreads;
  static std::unique_ptr<PostgreSQL> evidenceDatabase;
  static std::string alertTableName;
};

#endif  // #ifndef __EVIDENCE_PROCESSOR_HEADER_H__

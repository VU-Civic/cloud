#ifndef __EVIDENCE_PROCESSOR_HEADER_H__
#define __EVIDENCE_PROCESSOR_HEADER_H__

#include <unordered_map>
#include <vector>

class EvidenceProcessor final
{
public:

  // Initialization/cleanup
  static void initialize(void);
  static void cleanup(void);

  // Evidence processing functions
  static void processEvidenceData(std::unordered_map<uint32_t, std::vector<std::vector<uint8_t>>>::node_type&& evidence);

private:

  // Thread worker functions
  static void processEvidenceWorker(uint32_t deviceID, std::vector<uint8_t>&& rawEvidence);

  // Private member variables
  static std::atomic<uint32_t> numActiveThreads;
  static std::string evidenceClipBaseUrl;
  static const size_t frameHeaderSize;
};

#endif  // #ifndef __EVIDENCE_PROCESSOR_HEADER_H__

#ifndef __FUSION_ALGORITHM_HEADER_H__
#define __FUSION_ALGORITHM_HEADER_H__

#include <vector>
#include "Common.h"
#include "WeatherRetriever.h"

class Fusion final
{
public:

  // Initialization/cleanup
  static void initialize(const char* owmApiKey, const char* tomorrowIoApiKey);
  static void cleanup(void);

  // Packet feasibility with respect to a validation packet
  static bool validateFeasibility(const GunshotReport* validationPacket, const GunshotReport* packet);

  // Fusion algorithm
  static IncidentMessage performFusion(const std::vector<std::shared_ptr<GunshotReport>>& packetBundle);

private:

  // Helper functions for the fusion algorithm
  static IncidentMessage localizeUsingHoughTransform(float localSpeedOfSound, const std::vector<std::shared_ptr<GunshotReport>>& packetBundle);

  // Private member variables
  static WeatherRetriever weatherRetriever;
};

#endif  // #ifndef __FUSION_ALGORITHM_HEADER_H__

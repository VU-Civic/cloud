#ifndef __FUSION_ALGORITHM_HEADER_H__
#define __FUSION_ALGORITHM_HEADER_H__

#include <map>
#include "Common.h"

class Fusion final
{
public:

  // Initialization/cleanup
  static void initialize(void);
  static void cleanup(void);

  // Packet feasibility with respect to a validation packet
  static bool validateFeasibility(const std::map<uint32_t, std::shared_ptr<AlertMessage>>& packetBundle, const AlertMessage* validationPacket, const AlertMessage* packet);

  // Fusion algorithm
  static IncidentMessage performFusion(std::map<uint32_t, std::shared_ptr<AlertMessage>>& packetBundle);

private:
};

#endif  // #ifndef __FUSION_ALGORITHM_HEADER_H__

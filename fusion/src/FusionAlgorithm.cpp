#include "FusionAlgorithm.h"
#include "GeoMath.h"

void Fusion::initialize(void) { ; }

void Fusion::cleanup(void) { ; }

bool Fusion::validateFeasibility(const std::map<uint32_t, std::shared_ptr<AlertMessage>>& packetBundle, const AlertMessage* validationPacket, const AlertMessage* packet)
{
  return (packetBundle.count(packet->deviceID) == 0) && ((packet->timestamp - validationPacket->timestamp) <= CivicAlert::FUSION_MAX_POSSIBLE_TIME_DIFFERENCE_SECONDS) &&
         (GeoMath::calculateDistance3D(validationPacket->lat, validationPacket->lon, validationPacket->height, packet->lat, packet->lon, packet->height) <
          CivicAlert::FUSION_MAX_POSSIBLE_DISTANCE_METERS);
  // TODO: CAN WE ALSO VALIDATE BASED ON ACTUAL KNOWN DISTANCE BETWEEN THE TWO SENSORS TO CONSTRAIN THE TIME DIFFERENCE (THEN TOTAL DELETE MAX_POSSIBLE_TIME_DIFFERENCE)
}

IncidentMessage Fusion::performFusion(std::map<uint32_t, std::shared_ptr<AlertMessage>>& packetBundle)
{
  logger.log(Logger::INFO, "Performing fusion on %zu packets\n", packetBundle.size());
  IncidentMessage result;
  return result;
}

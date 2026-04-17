#ifndef __HOUGH_TRANSFORM_HEADER_H__
#define __HOUGH_TRANSFORM_HEADER_H__

#include <bitset>
#include <memory>
#include <tuple>
#include <vector>
#include "Common.h"

class HoughTransform final
{
public:

  // Constructors/destructor
  HoughTransform(void);
  ~HoughTransform(void);
  void reset2D(float xMin, float xMax, float yMin, float yMax, float tMin, float tMax, float resolutionMeters, float estimatedSpeedOfSound);
  void reset3D(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, float tMin, float tMax, float resolutionMeters, float estimatedSpeedOfSound);

  // Tranformation functions
  void transformAngleOfArrival(float aoaVectorX, float aoaVectorY, float aoaVectorZ, float unitLocationX, float unitLocationY, float unitLocationZ, int supporterIndex,
                               float maximumRangeMeters);
  void transformAngleOfArrival(float toaSeconds, float aoaVectorX, float aoaVectorY, float aoaVectorZ, float unitLocationX, float unitLocationY, float unitLocationZ,
                               int supporterIndex);
  void transformTimeOfArrival(float toaSeconds, float unitLocationX, float unitLocationY, float unitLocationZ, int supporterIndex);

  // Peak-finding function
  std::vector<std::pair<std::tuple<int, int, int, int>, std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>>> locateMaximumPeaks(
      size_t minimumSupportThreshold, const std::vector<std::shared_ptr<GunshotReport>>& gslReports);
  std::vector<std::pair<std::tuple<int, int, int, int>, std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>>> locatePeaks(size_t minimumSupportThreshold);

  // Bin distance and transformation functions
  float calculateBinDistanceFromCenter(int xBin, int yBin, int zBin) const;
  std::tuple<float, float, float, float> convertBinsToXYZT(int xBin, int yBin, int zBin, int tBin, bool centerOfBin);

private:

  // Helper functions
  std::tuple<int, int, int, int> findContainingBins(float x, float y, float z, float t) const;
  int findContainingXBin(float x) const;
  int findContainingYBin(float y) const;
  int findContainingZBin(float z) const;
  int findContainingTimeBin(float t) const;
  int findBinDistance(float actualDistance) const;
  void increaseBinSupport(int xBin, int yBin, int zBin, int tBin, int supporterIndex);
  void increaseBinSupportInCircle(float xCenter, float yCenter, float zCenter, float t, float radius, int supporterIndex);
  void increaseBinSupportInSphere(float xCenter, float yCenter, float zCenter, float t, float radius, int supporterIndex);
  static size_t countNonoverlappingSupporters(const std::vector<std::shared_ptr<GunshotReport>>& gslReports,
                                              const std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>& supportBitmap);

  // Private member variables
  bool is3D;
  float minX, minY, minZ, maxX, maxY, maxZ;
  float minT, maxT, timeResolution;
  float posResolution, speedOfSound;
  int xNumBins, yNumBins, zNumBins, tNumBins;
  std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>**** accumulatorBins;
};

#endif  // #ifndef __HOUGH_TRANSFORM_HEADER_H__

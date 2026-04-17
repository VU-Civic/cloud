#include <numeric>
#include "HoughTransform.h"

static const float sinTable[] = {
    0.0,      0.017452, 0.034899, 0.052336, 0.069756, 0.087156, 0.104528, 0.121869, 0.139173, 0.156434, 0.173648, 0.190809, 0.207912, 0.224951, 0.241922, 0.258819, 0.275637,
    0.292372, 0.309017, 0.325568, 0.342020, 0.358368, 0.374607, 0.390731, 0.406737, 0.422618, 0.438371, 0.453990, 0.469472, 0.484810, 0.5,      0.515038, 0.529919, 0.544639,
    0.559193, 0.573576, 0.587785, 0.601815, 0.615661, 0.629320, 0.642788, 0.656059, 0.669131, 0.681998, 0.694658, 0.707107, 0.719340, 0.731354, 0.743145, 0.754710, 0.766044,
    0.777146, 0.788011, 0.798636, 0.809017, 0.819152, 0.829038, 0.838671, 0.848048, 0.857167, 0.866025, 0.874620, 0.882948, 0.891007, 0.898794, 0.906308, 0.913545, 0.920505,
    0.927184, 0.933580, 0.939693, 0.945519, 0.951057, 0.956305, 0.961262, 0.965926, 0.970296, 0.974370, 0.978148, 0.981627, 0.984808, 0.987688, 0.990268, 0.992546, 0.994522,
    0.996195, 0.997564, 0.998630, 0.999391, 0.999848, 1.0,      0.999848, 0.999391, 0.998630, 0.997564, 0.996195, 0.994522, 0.992546, 0.990268, 0.987688, 0.984808, 0.981627,
    0.978148, 0.974370, 0.970296, 0.965926, 0.961262, 0.956305, 0.951057, 0.945519, 0.939693, 0.933580, 0.927184, 0.920505, 0.913545, 0.906308, 0.898794, 0.891007, 0.882948,
    0.874620, 0.866025, 0.857167, 0.848048, 0.838671, 0.829038, 0.819152, 0.809017, 0.798636, 0.788011, 0.777146, 0.766044, 0.754710, 0.743145, 0.731354, 0.719340, 0.707107,
    0.694658, 0.681998, 0.669131, 0.656059, 0.642788, 0.629320, 0.615661, 0.601815, 0.587785, 0.573576, 0.559193, 0.544639, 0.529919, 0.515038, 0.5,      0.484810, 0.469472,
    0.453990, 0.438371, 0.422618, 0.406737, 0.390731, 0.374607, 0.358368, 0.342020, 0.325568, 0.309017, 0.292372, 0.275637, 0.258819, 0.241922, 0.224951, 0.207912, 0.190809,
    0.173648, 0.156434, 0.139173, 0.121869, 0.104528, 0.087156, 0.069756, 0.052336, 0.034899, 0.017452};

HoughTransform::HoughTransform(void)
    : is3D(true),
      minX(0.0f),
      minY(0.0f),
      minZ(0.0f),
      maxX(0.0f),
      maxY(0.0f),
      maxZ(0.0f),
      minT(0.0f),
      maxT(0.0f),
      timeResolution(0.0f),
      posResolution(0.0f),
      speedOfSound(SPEED_OF_SOUND_IN_AIR_METERS_PER_SECOND()),
      xNumBins(0),
      yNumBins(0),
      zNumBins(0),
      tNumBins(0),
      accumulatorBins(nullptr)
{
}
HoughTransform::~HoughTransform(void)
{
  // Clean up the multidimensional accumulation array
  for (int i = 0; i < xNumBins; ++i)
  {
    for (int j = 0; j < yNumBins; ++j)
    {
      for (int k = 0; k < zNumBins; ++k) delete[] accumulatorBins[i][j][k];
      delete[] accumulatorBins[i][j];
    }
    delete[] accumulatorBins[i];
  }
  if (accumulatorBins) delete[] accumulatorBins;
}

void HoughTransform::reset2D(float xMin, float xMax, float yMin, float yMax, float tMin, float tMax, float resolutionMeters, float estimatedSpeedOfSound)
{
  reset3D(xMin, xMax, yMin, yMax, -0.5f * resolutionMeters, (0.5f * resolutionMeters) + 0.1f, tMin, tMax, resolutionMeters, estimatedSpeedOfSound);
  is3D = false;
}

void HoughTransform::reset3D(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, float tMin, float tMax, float resolutionMeters, float estimatedSpeedOfSound)
{
  // Clean up the current multidimensional accumulation array
  for (int i = 0; i < xNumBins; ++i)
  {
    for (int j = 0; j < yNumBins; ++j)
    {
      for (int k = 0; k < zNumBins; ++k) delete[] accumulatorBins[i][j][k];
      delete[] accumulatorBins[i][j];
    }
    delete[] accumulatorBins[i];
  }
  if (accumulatorBins) delete[] accumulatorBins;

  // Reset the Hough Transform parameters
  minX = xMin;
  minY = yMin;
  minZ = zMin;
  minT = tMin;
  maxX = xMax;
  maxY = yMax;
  maxZ = zMax;
  maxT = tMax;
  posResolution = resolutionMeters;
  timeResolution = resolutionMeters / estimatedSpeedOfSound;
  speedOfSound = estimatedSpeedOfSound;
  xNumBins = static_cast<int>((xMax - xMin) / posResolution);
  yNumBins = static_cast<int>((yMax - yMin) / posResolution);
  zNumBins = static_cast<int>((zMax - zMin) / posResolution);
  tNumBins = static_cast<int>((tMax - tMin) / timeResolution);
  is3D = true;

  // Create a new multidimensional accumulation array
  accumulatorBins = new std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>***[xNumBins];
  for (int i = 0; i < xNumBins; ++i)
  {
    accumulatorBins[i] = new std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>**[yNumBins];
    for (int j = 0; j < yNumBins; ++j)
    {
      accumulatorBins[i][j] = new std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>*[zNumBins];
      for (int k = 0; k < zNumBins; ++k) accumulatorBins[i][j][k] = new std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>[tNumBins];
    }
  }
}

void HoughTransform::transformAngleOfArrival(float aoaVectorX, float aoaVectorY, float aoaVectorZ, float unitLocationX, float unitLocationY, float unitLocationZ,
                                             int supporterIndex, float maximumRangeMeters)
{
  // Iterate through all possible shot times
  for (float t = minT; t <= maxT; t += timeResolution)
  {
    // Increase all accumulation bins through which this line intersects
    int xBin, yBin, zBin, tBin;
    int minPossibleRange = 0;
    int maxPossibleRange = findBinDistance(maximumRangeMeters);

    for (int range = minPossibleRange; range <= maxPossibleRange; ++range)
    {
      std::tie(xBin, yBin, zBin, tBin) = findContainingBins(unitLocationX + (range * posResolution * aoaVectorX), unitLocationY + (range * posResolution * aoaVectorY),
                                                            unitLocationZ + (range * posResolution * aoaVectorZ), t);
      increaseBinSupport(xBin, yBin, is3D ? zBin : 0, tBin, supporterIndex);
    }
  }
}

void HoughTransform::transformAngleOfArrival(float toaSeconds, float aoaVectorX, float aoaVectorY, float aoaVectorZ, float unitLocationX, float unitLocationY, float unitLocationZ,
                                             int supporterIndex)
{
  // Iterate through all possible shot times
  float halfTimeResolution = 0.5f * timeResolution;
  for (float t = minT; t <= maxT; t += halfTimeResolution)
  {
    // Increase the accumulation bins through which this line intersects around time = t
    int xBin, yBin, zBin, tBin;
    float tBasedRange = speedOfSound * (toaSeconds - t);
    if ((tBasedRange - posResolution) >= 0.0)
    {
      std::tie(xBin, yBin, zBin, tBin) =
          findContainingBins(unitLocationX + ((tBasedRange - posResolution) * aoaVectorX), unitLocationY + ((tBasedRange - posResolution) * aoaVectorY),
                             unitLocationZ + ((tBasedRange - posResolution) * aoaVectorZ), t);
      increaseBinSupport(xBin, yBin, is3D ? zBin : 0, tBin, supporterIndex);
    }
    std::tie(xBin, yBin, zBin, tBin) =
        findContainingBins(unitLocationX + (tBasedRange * aoaVectorX), unitLocationY + (tBasedRange * aoaVectorY), unitLocationZ + (tBasedRange * aoaVectorZ), t);
    increaseBinSupport(xBin, yBin, is3D ? zBin : 0, tBin, supporterIndex);
    std::tie(xBin, yBin, zBin, tBin) =
        findContainingBins(unitLocationX + ((tBasedRange + posResolution) * aoaVectorX), unitLocationY + ((tBasedRange + posResolution) * aoaVectorY),
                           unitLocationZ + ((tBasedRange + posResolution) * aoaVectorZ), t);
    increaseBinSupport(xBin, yBin, is3D ? zBin : 0, tBin, supporterIndex);
  }
}

void HoughTransform::transformTimeOfArrival(float toaSeconds, float unitLocationX, float unitLocationY, float unitLocationZ, int supporterIndex)
{
  // Iterate through all possible shot times
  for (float t = minT; t <= maxT; t += timeResolution)
  {
    // Increase the accumulation bins through which this circle/sphere intersects
    if (is3D)
    {
      increaseBinSupportInSphere(unitLocationX, unitLocationY, unitLocationZ, t, speedOfSound * (toaSeconds - t) - posResolution, supporterIndex);
      increaseBinSupportInSphere(unitLocationX, unitLocationY, unitLocationZ, t, speedOfSound * (toaSeconds - t), supporterIndex);
      increaseBinSupportInSphere(unitLocationX, unitLocationY, unitLocationZ, t, speedOfSound * (toaSeconds - t) + posResolution, supporterIndex);
    }
    else
    {
      increaseBinSupportInCircle(unitLocationX, unitLocationY, unitLocationZ, t, speedOfSound * (toaSeconds - t) - posResolution, supporterIndex);
      increaseBinSupportInCircle(unitLocationX, unitLocationY, unitLocationZ, t, speedOfSound * (toaSeconds - t), supporterIndex);
      increaseBinSupportInCircle(unitLocationX, unitLocationY, unitLocationZ, t, speedOfSound * (toaSeconds - t) + posResolution, supporterIndex);
    }
  }
}

std::vector<std::pair<std::tuple<int, int, int, int>, std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>>> HoughTransform::locateMaximumPeaks(
    size_t minimumSupportThreshold, const std::vector<std::shared_ptr<GunshotReport>>& gslReports)
{
  std::vector<std::pair<std::tuple<int, int, int, int>, std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>>> maxPeaks;
  size_t maxNumSupporters = 0, numSupporters;
  for (int xBin = 0; xBin < xNumBins; ++xBin)
    for (int yBin = 0; yBin < yNumBins; ++yBin)
      for (int zBin = 0; zBin < zNumBins; ++zBin)
        for (int tBin = 0; tBin < tNumBins; ++tBin)
        {
          numSupporters = accumulatorBins[xBin][yBin][zBin][tBin].count();
          if ((numSupporters >= minimumSupportThreshold) && (numSupporters >= maxNumSupporters))
          {
            numSupporters = countNonoverlappingSupporters(gslReports, accumulatorBins[xBin][yBin][zBin][tBin]);
            if ((numSupporters >= minimumSupportThreshold) && (numSupporters > maxNumSupporters))
            {
              maxNumSupporters = numSupporters;
              maxPeaks.clear();
              maxPeaks.emplace_back(std::make_tuple(xBin, yBin, zBin, tBin), accumulatorBins[xBin][yBin][zBin][tBin]);
            }
            else if (numSupporters == maxNumSupporters)
              maxPeaks.emplace_back(std::make_tuple(xBin, yBin, zBin, tBin), accumulatorBins[xBin][yBin][zBin][tBin]);
          }
        }
  return maxPeaks;
}

std::vector<std::pair<std::tuple<int, int, int, int>, std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>>> HoughTransform::locatePeaks(size_t minimumSupportThreshold)
{
  std::vector<std::pair<std::tuple<int, int, int, int>, std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>>> peaks;
  for (int xBin = 0; xBin < xNumBins; ++xBin)
    for (int yBin = 0; yBin < yNumBins; ++yBin)
      for (int zBin = 0; zBin < zNumBins; ++zBin)
        for (int tBin = 0; tBin < tNumBins; ++tBin)
          if (accumulatorBins[xBin][yBin][zBin][tBin].count() >= minimumSupportThreshold)
            peaks.emplace_back(std::make_tuple(xBin, yBin, zBin, tBin), accumulatorBins[xBin][yBin][zBin][tBin]);
  return peaks;
}

float HoughTransform::calculateBinDistanceFromCenter(int xBin, int yBin, int zBin) const
{
  float xBinDistance = (xNumBins * 0.5f) - xBin;
  float yBinDistance = (yNumBins * 0.5f) - yBin;
  float zBinDistance = (zNumBins * 0.5f) - zBin;
  return std::sqrt(xBinDistance * xBinDistance + yBinDistance * yBinDistance + zBinDistance * zBinDistance);
}

std::tuple<float, float, float, float> HoughTransform::convertBinsToXYZT(int xBin, int yBin, int zBin, int tBin, bool centerOfBin)
{
  float binCenterPosOffset = (centerOfBin ? (0.5f * posResolution) : 0.0f);
  float binCenterTimeOffset = (centerOfBin ? (0.5 * timeResolution) : 0.0);
  return std::make_tuple(minX + (xBin * posResolution) + binCenterPosOffset, minY + (yBin * posResolution) + binCenterPosOffset, minZ + (zBin * posResolution) + binCenterPosOffset,
                         minT + (tBin * timeResolution) + binCenterTimeOffset);
}

std::tuple<int, int, int, int> HoughTransform::findContainingBins(float x, float y, float z, float t) const
{
  return std::make_tuple(static_cast<int>((x - minX) / posResolution), static_cast<int>((y - minY) / posResolution), static_cast<int>((z - minZ) / posResolution),
                         static_cast<int>((t - minT) / timeResolution));
}

int HoughTransform::findContainingXBin(float x) const { return static_cast<int>((x - minX) / posResolution); }

int HoughTransform::findContainingYBin(float y) const { return static_cast<int>((y - minY) / posResolution); }

int HoughTransform::findContainingZBin(float z) const { return static_cast<int>((z - minZ) / posResolution); }

int HoughTransform::findContainingTimeBin(float t) const { return static_cast<int>((t - minT) / timeResolution); }

int HoughTransform::findBinDistance(float actualDistance) const { return static_cast<int>(actualDistance / posResolution); }

void HoughTransform::increaseBinSupport(int xBin, int yBin, int zBin, int tBin, int supporterIndex)
{
  // Set the support bit in the corresponding accumulation bin if it is within range
  if ((xBin >= 0) && (xBin < xNumBins) && (yBin >= 0) && (yBin < yNumBins) && (zBin >= 0) && (zBin < zNumBins) && (tBin >= 0) && (tBin < tNumBins))
    accumulatorBins[xBin][yBin][zBin][tBin].set(supporterIndex);
}

void HoughTransform::increaseBinSupportInCircle(float xCenter, float yCenter, float zCenter, float t, float radius, int supporterIndex)
{
  // Only continue if the radius is valid
  if (radius < 0.0f) return;

  // Increase the accumulation bins in a circle around the center point
  for (int i = 0; i < 45; ++i)
  {
    float xOffset = (radius * sinTable[90 + i]);
    float yOffset = (radius * sinTable[i]);
    int xBin1 = findContainingXBin(xCenter + xOffset), xBin2 = findContainingXBin(xCenter - xOffset), xBin3 = findContainingXBin(xCenter + yOffset),
        xBin4 = findContainingXBin(xCenter - yOffset);
    int yBin1 = findContainingYBin(yCenter + yOffset), yBin2 = findContainingYBin(yCenter - yOffset), yBin3 = findContainingYBin(yCenter + xOffset),
        yBin4 = findContainingYBin(yCenter - xOffset);
    int zBin = findContainingZBin(zCenter), tBin = findContainingTimeBin(t);
    increaseBinSupport(xBin1, yBin1, is3D ? zBin : 0, tBin, supporterIndex);
    increaseBinSupport(xBin1, yBin2, is3D ? zBin : 0, tBin, supporterIndex);
    increaseBinSupport(xBin2, yBin1, is3D ? zBin : 0, tBin, supporterIndex);
    increaseBinSupport(xBin2, yBin2, is3D ? zBin : 0, tBin, supporterIndex);
    increaseBinSupport(xBin3, yBin3, is3D ? zBin : 0, tBin, supporterIndex);
    increaseBinSupport(xBin3, yBin4, is3D ? zBin : 0, tBin, supporterIndex);
    increaseBinSupport(xBin4, yBin3, is3D ? zBin : 0, tBin, supporterIndex);
    increaseBinSupport(xBin4, yBin4, is3D ? zBin : 0, tBin, supporterIndex);
  }
}

void HoughTransform::increaseBinSupportInSphere(float xCenter, float yCenter, float zCenter, float t, float radius, int supporterIndex)
{
  // Increase the accumulation bins in a sphere around the center point
  for (int i = 0; i < 90; ++i)
  {
    float zOffset = (radius * sinTable[i]);
    float newRadius = (radius * sinTable[90 + i]);
    increaseBinSupportInCircle(xCenter, yCenter, zCenter + zOffset, t, newRadius, supporterIndex);
    increaseBinSupportInCircle(xCenter, yCenter, zCenter - zOffset, t, newRadius, supporterIndex);
  }
}

size_t HoughTransform::countNonoverlappingSupporters(const std::vector<std::shared_ptr<GunshotReport>>& gslReports,
                                                     const std::bitset<2 * CivicAlert::FUSION_MAX_NUM_EVENTS>& supportBitmap)
{
  // Store the maximum number of supporters for each node and return the cumulative total
  std::unordered_map<std::string, size_t> maxSupporterCounts;
  for (size_t i = 0; i < gslReports.size(); ++i)
  {
    size_t supportCount = supportBitmap.test(i) ? 1 : 0;
    if (supportBitmap.test(i + CivicAlert::FUSION_MAX_NUM_EVENTS)) ++supportCount;
    // TODO: maxSupporterCounts[gslReports[i]->nodeID] = std::max(maxSupporterCounts[gslReports[i]->nodeID], supportCount);
  }
  return std::accumulate(std::begin(maxSupporterCounts), std::end(maxSupporterCounts), 0,
                         [](const size_t& sum, const std::pair<std::string, size_t>& count) { return sum + count.second; });
}

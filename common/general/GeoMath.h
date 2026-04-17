#ifndef __GEO_MATH_HEADER_H__
#define __GEO_MATH_HEADER_H__

#include <cstdint>
#include <tuple>

class GeoMath final
{
public:

  static std::tuple<float, float, float> convertLlhToEcef(float latDegrees, float lonDegrees, float heightMeters);
  static std::tuple<float, float, float> convertEcefToEnu(float refLat, float refLon, float refHeight, float x, float y, float z);
  static std::tuple<float, float, float> convertEnuToEcef(float refLat, float refLon, float refHeight, float east, float north, float up);
  static std::tuple<float, float, float> convertEcefToLlh(float x, float y, float z);
  static float calculateEcefDistance(float x1, float y1, float z1, float x2, float y2, float z2);
  static float calculateDistance2D(float latDegrees1, float lonDegrees1, float latDegrees2, float lonDegrees2);
  static float calculateDistance3D(float latDegrees1, float lonDegrees1, float heightMeters1, float latDegrees2, float lonDegrees2, float heightMeters2);
  static float estimateDistance2D(float lat1Radians, float lon1Radians, float lat2Radians, float lon2Radians);
  static std::tuple<float, float, float, float> calculateFullQuaternion(int32_t q1, int32_t q2, int32_t q3);
  static std::tuple<float, float> calculateSourceDirection(float qw, float qx, float qy, float qz, const float aoa[3]);
};

#endif  // #ifndef __GEO_MATH_HEADER_H__

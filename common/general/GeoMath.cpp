#include "GeoMath.h"
#include "MathUtilities.h"

static constexpr const float A1 = Math::EARTH_EQUATORIAL_RADIUS * Math::EARTH_ECCENTRICITY_2;
static constexpr const float A2 = A1 * A1;
static constexpr const float A3 = 0.5 * Math::EARTH_EQUATORIAL_RADIUS * Math::EARTH_ECCENTRICITY_2 * Math::EARTH_ECCENTRICITY_2;
static constexpr const float A4 = (5.0 / 2.0) * A2;
static constexpr const float A5 = A1 + A3;
static constexpr const float A6 = 1.0 - Math::EARTH_ECCENTRICITY_2;

std::tuple<float, float, float> GeoMath::convertLlhToEcef(float latDegrees, float lonDegrees, float heightMeters)
{
  const auto cosLat = std::cos(latDegrees * Math::DEGREES_TO_RADIANS), sinLat = std::sin(latDegrees * Math::DEGREES_TO_RADIANS);
  const auto N = Math::EARTH_EQUATORIAL_RADIUS / std::sqrt(1.0 - (Math::EARTH_ECCENTRICITY_2 * sinLat * sinLat));
  const auto commonParam = (N + heightMeters) * cosLat;
  const auto x = commonParam * std::cos(lonDegrees * Math::DEGREES_TO_RADIANS);
  const auto y = commonParam * std::sin(lonDegrees * Math::DEGREES_TO_RADIANS);
  const auto z = ((N * (1.0 - Math::EARTH_ECCENTRICITY_2)) + heightMeters) * sinLat;
  return std::make_tuple(x, y, z);
}

std::tuple<float, float, float> GeoMath::convertEcefToEnu(float refLat, float refLon, float refHeight, float x, float y, float z)
{
  const auto cosLat = std::cos(refLat * Math::DEGREES_TO_RADIANS), sinLat = std::sin(refLat * Math::DEGREES_TO_RADIANS);
  const auto cosLon = std::cos(refLon * Math::DEGREES_TO_RADIANS), sinLon = std::sin(refLon * Math::DEGREES_TO_RADIANS);
  const auto N = Math::EARTH_EQUATORIAL_RADIUS / std::sqrt(1.0 - (Math::EARTH_ECCENTRICITY_2 * sinLat * sinLat));
  const auto commonParam = (N + refHeight) * cosLat;
  const auto refX = commonParam * cosLon;
  const auto refY = commonParam * sinLon;
  const auto refZ = ((N * (1.0 - Math::EARTH_ECCENTRICITY_2)) + refHeight) * sinLat;
  const auto xd = x - refX, yd = y - refY, zd = z - refZ;
  const auto east = -sinLon * xd + cosLon * yd;
  const auto north = -cosLon * sinLat * xd - sinLat * sinLon * yd + cosLat * zd;
  const auto up = cosLat * cosLon * xd + cosLat * sinLon * yd + sinLat * zd;
  return std::make_tuple(east, north, up);
}

std::tuple<float, float, float> GeoMath::convertEnuToEcef(float refLat, float refLon, float refHeight, float east, float north, float up)
{
  const auto cosLat = std::cos(refLat * Math::DEGREES_TO_RADIANS), sinLat = std::sin(refLat * Math::DEGREES_TO_RADIANS);
  const auto cosLon = std::cos(refLon * Math::DEGREES_TO_RADIANS), sinLon = std::sin(refLon * Math::DEGREES_TO_RADIANS);
  const auto N = Math::EARTH_EQUATORIAL_RADIUS / std::sqrt(1.0 - (Math::EARTH_ECCENTRICITY_2 * sinLat * sinLat));
  const auto commonParam = (N + refHeight) * cosLat;
  const auto refX = commonParam * cosLon;
  const auto refY = commonParam * sinLon;
  const auto refZ = ((N * (1.0 - Math::EARTH_ECCENTRICITY_2)) + refHeight) * sinLat;
  const auto x = refX + (-sinLon * east - sinLat * cosLon * north + cosLat * cosLon * up);
  const auto y = refY + (cosLon * east - sinLat * sinLon * north + cosLat * sinLon * up);
  const auto z = refZ + (cosLat * north + sinLat * up);
  return std::make_tuple(x, y, z);
}

std::tuple<float, float, float> GeoMath::convertEcefToLlh(float x, float y, float z)
{
  const auto positiveZ = std::abs(z), W2 = (x * x + y * y), Z2 = z * z;
  const auto W = std::sqrt(W2), R2 = W2 + Z2;
  const auto R = std::sqrt(R2);
  auto S = positiveZ / R, C = W / R, U = A2 / R, V = A3 - (A4 / R);
  auto S2 = S * S, C2 = C * C;

  // Compute longitude
  auto lat = 0.0f;
  auto lon = std::atan2(y, x);

  // Compute latitude differently depending on its nearness to the Earth's poles
  if (C2 > 0.3)
  {
    S *= (1.0 + C2 * (A1 + U + S2 * V) / R);
    lat = std::asin(S);
    S2 = S * S;
    C = std::sqrt(1.0 - S2);
  }
  else
  {
    C *= (1.0 - S2 * (A5 - U - C2 * V) / R);
    lat = std::acos(C);
    S2 = 1.0 - C * C;
    S = std::sqrt(S2);
  }

  // Compute height
  const auto G = 1.0 - Math::EARTH_ECCENTRICITY_2 * S2;
  const auto R1 = Math::EARTH_EQUATORIAL_RADIUS / std::sqrt(G);
  const auto Rf = A6 * R1;
  U = W - R1 * C;
  V = positiveZ - Rf * S;
  const auto F = C * U + S * V, M = C * V - S * U;
  const auto P = M / (Rf / G + F);
  lat += P;
  const auto height = F + 0.5 * M * P;

  // Convert to degrees and return
  if (z < 0.0) lat = -lat;
  lat *= Math::RADIANS_TO_DEGREES;
  lon *= Math::RADIANS_TO_DEGREES;
  return std::make_tuple(lat, lon, height);
}

float GeoMath::calculateEcefDistance(float x1, float y1, float z1, float x2, float y2, float z2)
{
  return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
}

float GeoMath::calculateDistance2D(float latDegrees1, float lonDegrees1, float latDegrees2, float lonDegrees2)
{
  const auto [x1, y1, z1] = convertLlhToEcef(latDegrees1, lonDegrees1, 0.0);
  const auto [x2, y2, z2] = convertLlhToEcef(latDegrees2, lonDegrees2, 0.0);
  return calculateEcefDistance(x1, y1, z1, x2, y2, z2);
}

float GeoMath::calculateDistance3D(float latDegrees1, float lonDegrees1, float heightMeters1, float latDegrees2, float lonDegrees2, float heightMeters2)
{
  const auto [x1, y1, z1] = convertLlhToEcef(latDegrees1, lonDegrees1, heightMeters1);
  const auto [x2, y2, z2] = convertLlhToEcef(latDegrees2, lonDegrees2, heightMeters2);
  return calculateEcefDistance(x1, y1, z1, x2, y2, z2);
}

float GeoMath::estimateDistance2D(float lat1Radians, float lon1Radians, float lat2Radians, float lon2Radians)
{
  const auto x = lat2Radians - lat1Radians;
  const auto y = (lon2Radians - lon1Radians) * std::cos(lat1Radians);
  return Math::EARTH_EQUATORIAL_RADIUS * std::sqrt((x * x) + (y * y));
}

std::tuple<float, float, float, float> GeoMath::calculateFullQuaternion(int32_t q1, int32_t q2, int32_t q3)
{  // TODO: TRY ALIGNGING Q1->Z, Q2->X, Q3->Y and see if it matches the output of the current implementation
  const auto x = q2 / 1073741824.0f;
  const auto y = q3 / 1073741824.0f;
  const auto z = -q1 / 1073741824.0f;
  const auto w = std::sqrt(1.0f - (x * x + y * y + z * z));
  return std::make_tuple(w, x, y, z);
}

std::tuple<float, float> GeoMath::calculateSourceDirection(float qw, float qx, float qy, float qz, float aoa_x, float aoa_y, float aoa_z)
{
  // const float azimuth = atan2f(aoa[1], aoa[0]), elevation = asinf(aoa[2]);

  // Rotate the AoA vector from the sensor frame into the global ENU world frame
  const auto tx = 2.0f * (qy * aoa_z - qz * aoa_y);
  const auto ty = 2.0f * (qz * aoa_x - qx * aoa_z);
  const auto tz = 2.0f * (qx * aoa_y - qy * aoa_x);
  const float aoaGlobal[3] = {aoa_x + qw * tx + (qy * tz - qz * ty), aoa_y + qw * ty + (qz * tx - qx * tz), aoa_z + qw * tz + (qx * ty - qy * tx)};
  const auto horiz = sqrtf(aoaGlobal[0] * aoaGlobal[0] + aoaGlobal[1] * aoaGlobal[1]);
  const auto azimuth_enu = atan2f(aoaGlobal[1], aoaGlobal[0]);
  const auto elevation = atan2f(aoaGlobal[2], horiz);

  // TODO: FINALIZE THESE COMPUTATIONS
  // TEST: Compute roll (x-axis rotation)
  /*const auto t0 = qw * qx + qy * qz;
  const auto t1 = 0.5f - (qx * qx + qy * qy);
  const auto roll = atan2f(t0, t1) * 180.0f / (float)M_PI;
  // TEST: Compute pitch (y-axis rotation)
  const auto t2 = fmaxf(-1.0f, fminf(1.0f, 2.0f * (qw * qy - qx * qz)));
  const auto pitch = asinf(t2) * 180.0f / (float)M_PI;
  // TEST: Compute yaw (z-axis rotation)
  const auto t3 = qw * qz + qx * qy;
  const auto t4 = 0.5f - (qy * qy + qz * qz);
  const auto yaw = atan2f(t3, t4) * 180.0f / (float)M_PI;*/
  // printf("Calculated Sensor Orientation: Roll = %.4f deg, Pitch = %.4f deg, Yaw = %.4f deg\n", -(90.0f + roll), pitch, fmodf(yaw - 90.0f, 360.0f));

  // Rotate the azimuth from the ENU world frame to the NED world frame
  const auto bearing = (float)M_PI_2 - azimuth_enu;
  return std::make_tuple((bearing < 0.0f) ? (bearing + (float)(2.0 * M_PI)) : bearing, elevation);
}

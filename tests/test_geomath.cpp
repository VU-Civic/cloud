#include "Logger.h"
#include "GeoMath.h"
#include "MathUtilities.h"

// Global application logger
Logger logger("/tmp/civicalert.log", Logger::DEBUG);

void test_llh_to_ecef(void)
{
  const auto nashvilleLat = 36.1622f, nashvilleLon = -86.7744f, nashvilleHeight = 160.0f;
  const auto [x, y, z] = GeoMath::convertLlhToEcef(nashvilleLat, nashvilleLon, nashvilleHeight);
  if (std::abs(x - 290089.44f) > 1.0f || std::abs(y - (-5147360.5f)) > 1.0f || std::abs(z - 3742831.75f) > 1.0f)
    logger.log(Logger::ERROR, "LLH-to-ECEF Test FAILED: Expected ECEF = <290089.44, -5147360.50, 3742831.75>, Got ECEF = <%.2f, %.2f, %.2f>\n", x, y, z);
  else
    logger.log(Logger::INFO, "LLH-to-ECEF Test PASSED: LLH = <%.6f, %.6f, %.2f> -> ECEF = <%.2f, %.2f, %.2f>\n", nashvilleLat, nashvilleLon, nashvilleHeight, x, y, z);
}

void test_ecef_to_enu(void)
{
  const auto x = 290329.30f, y = -5147604.53f, z = 3742480.07f, refLat = 36.1622f, refLon = -86.7744f, refHeight = 160.0f;
  const auto [east, north, up] = GeoMath::convertEcefToEnu(refLat, refLon, refHeight, x, y, z);
  if (std::abs(east - 225.76f) > 1.0f || std::abs(north - (-435.68f)) > 1.0f || std::abs(up - 0.13f) > 1.0f)
    logger.log(Logger::ERROR, "ECEF-to-ENU Test FAILED: Expected ENU = <225.76, -435.68, 0.13>, Got ENU = <%.2f, %.2f, %.2f>\n", east, north, up);
  else
    logger.log(Logger::INFO, "ECEF-to-ENU Test PASSED: ECEF = <%.2f, %.2f, %.2f> -> ENU = <%.2f, %.2f, %.2f>\n", x, y, z, east, north, up);
}

void test_enu_to_ecef(void)
{
  const auto east = 225.76f, north = -435.68f, up = 0.13f, refLat = 36.1622f, refLon = -86.7744f, refHeight = 160.0f;
  const auto [x, y, z] = GeoMath::convertEnuToEcef(refLat, refLon, refHeight, east, north, up);
  if (std::abs(x - 290329.30f) > 1.0f || std::abs(y - (-5147604.53f)) > 1.0f || std::abs(z - 3742480.07f) > 1.0f)
    logger.log(Logger::ERROR, "ENU-to-ECEF Test FAILED: Expected ECEF = <290329.30, -5147604.53, 3742480.07>, Got ECEF = <%.2f, %.2f, %.2f>\n", x, y, z);
  else
    logger.log(Logger::INFO, "ENU-to-ECEF Test PASSED: ENU = <%.2f, %.2f, %.2f> -> ECEF = <%.2f, %.2f, %.2f>\n", east, north, up, x, y, z);
}

void test_ecef_to_llh(void)
{
  const auto nashvilleX = 290089.44f, nashvilleY = -5147360.5f, nashvilleZ = 3742831.75f;
  const auto [lat, lon, height] = GeoMath::convertEcefToLlh(nashvilleX, nashvilleY, nashvilleZ);
  if (std::abs(lat - 36.1622f) > 0.0001f || std::abs(lon - (-86.7744f)) > 0.0001f || std::abs(height - 160.0f) > 1.0f)
    logger.log(Logger::ERROR, "ECEF-to-LLH Test FAILED: Expected LLH = <36.1622, -86.7744, 160.00>, Got LLH = <%.6f, %.6f, %.2f>\n", lat, lon, height);
  else
    logger.log(Logger::INFO, "ECEF-to-LLH Test PASSED: ECEF = <%.2f, %.2f, %.2f> -> LLH = <%.6f, %.6f, %.2f>\n", nashvilleX, nashvilleY, nashvilleZ, lat, lon, height);
}

void test_ecef_distance(void)
{
  const auto x1 = 290089.44f, y1 = -5147360.5f, z1 = 3742831.75f, x2 = 290329.30f, y2 = -5147604.53f, z2 = 3742480.07f;
  const auto distance = GeoMath::calculateEcefDistance(x1, y1, z1, x2, y2, z2);
  if (std::abs(distance - 490.72f) > 1.0f)
    logger.log(Logger::ERROR, "ECEF Distance Test FAILED: Expected Distance = 490.72, Got Distance = %.2f\n", distance);
  else
    logger.log(Logger::INFO, "ECEF Distance Test PASSED: ECEF1 = <%.2f, %.2f, %.2f>, ECEF2 = <%.2f, %.2f, %.2f> -> Distance = %.2f\n", x1, y1, z1, x2, y2, z2, distance);
}

void test_distance_2d(void)
{
  const auto lat1 = 36.1622f, lon1 = -86.7744f, lat2 = 36.158276f, lon2 = -86.771889f;
  const auto distance = GeoMath::calculateDistance2D(lat1, lon1, lat2, lon2);
  if (std::abs(distance - 491.0f) > 1.0f)
    logger.log(Logger::ERROR, "2D Distance Test FAILED: Expected Distance = 491.0, Got Distance = %.2f\n", distance);
  else
    logger.log(Logger::INFO, "2D Distance Test PASSED: LLH1 = <%.6f, %.6f>, LLH2 = <%.6f, %.6f> -> Distance = %.2f\n", lat1, lon1, lat2, lon2, distance);
}

void test_distance_3d(void)
{
  const auto lat1 = 36.1622f, lon1 = -86.7744f, height1 = 160.0f, lat2 = 36.158276f, lon2 = -86.771889f, height2 = 150.0f;
  const auto distance = GeoMath::calculateDistance3D(lat1, lon1, height1, lat2, lon2, height2);
  if (std::abs(distance - 491.0f) > 1.0f)
    logger.log(Logger::ERROR, "3D Distance Test FAILED: Expected Distance = 491.0, Got Distance = %.2f\n", distance);
  else
    logger.log(Logger::INFO, "3D Distance Test PASSED: LLH1 = <%.6f, %.6f, %.2f>, LLH2 = <%.6f, %.6f, %.2f> -> Distance = %.2f\n", lat1, lon1, height1, lat2, lon2, height2,
               distance);
}

void test_estimate_distance_2d(void)
{
  const auto lat1 = 36.1622f, lon1 = -86.7744f, lat2 = 36.158276f, lon2 = -86.771889f;
  const auto distance =
      GeoMath::estimateDistance2D(lat1 * Math::DEGREES_TO_RADIANS, lon1 * Math::DEGREES_TO_RADIANS, lat2 * Math::DEGREES_TO_RADIANS, lon2 * Math::DEGREES_TO_RADIANS);
  if (std::abs(distance - 491.0f) > 1.0f)
    logger.log(Logger::ERROR, "2D Distance Estimation Test FAILED: Expected Distance = 491.0, Got Distance = %.2f\n", distance);
  else
    logger.log(Logger::INFO, "2D Distance Estimation Test PASSED: LLH1 = <%.6f, %.6f>, LLH2 = <%.6f, %.6f> -> Distance = %.2f\n", lat1, lon1, lat2, lon2, distance);
}

void test_calculate_source_direction(void)
{
  const int32_t sensorQ1 = 695740479, sensorQ2 = -290263255, sensorQ3 = -268380970;
  const float aoa[3] = {0.0f, 1.0f, 0.0f};  // Source is directly in front of sensor
  const auto [qw, qx, qy, qz] = GeoMath::calculateFullQuaternion(sensorQ1, sensorQ2, sensorQ3);
  const auto [azimuth, elevation] = GeoMath::calculateSourceDirection(qw, qx, qy, qz, aoa[0], aoa[1], aoa[2]);
  printf("Calculated Source Direction: Azimuth = %.4f deg, Elevation = %.4f deg\n", azimuth * (180.0f / (float)M_PI), elevation * (180.0f / (float)M_PI));
  /*if (std::abs(azimuth - 0.0f) > 0.01f || std::abs(elevation - 1.5708f) > 0.01f)
    logger.log(Logger::ERROR, "Calculate Source Direction Test FAILED: Expected Azimuth = 0.00 rad, Elevation = 1.5708 rad, Got Azimuth = %.4f rad, Elevation = %.4f rad\n",
               azimuth, elevation);
  else
    logger.log(Logger::INFO, "Calculate Source Direction Test PASSED: Sensor Q1 = %d, Q2 = %d, Q3 = %d, AoA = <%.2f, %.2f, %.2f> -> Azimuth = %.4f rad, Elevation = %.4f rad\n",
               sensorQ1, sensorQ2, sensorQ3, aoa[0], aoa[1], aoa[2], azimuth, elevation);*/
}

int main(void)
{
  // Execute all GeoMath tests
  test_llh_to_ecef();
  test_ecef_to_enu();
  test_enu_to_ecef();
  test_ecef_to_llh();
  test_ecef_distance();
  test_distance_2d();
  test_estimate_distance_2d();
  test_distance_3d();
  test_calculate_source_direction();
  return 0;
}

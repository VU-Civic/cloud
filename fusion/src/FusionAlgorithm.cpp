#include "FusionAlgorithm.h"
#include "GeoMath.h"
#include "Logger.h"
#include "MathUtilities.h"

WeatherRetriever Fusion::weatherRetriever;

void Fusion::initialize(const char* owmApiKey, const char* tomorrowIoApiKey)
{
  // Set up the weather retriever with all known public APIs
  weatherRetriever.addPublicApiService(WeatherDataRetriever::PublicAPI::NOAA, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                       PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, nullptr, 1);
  weatherRetriever.addPublicApiService(WeatherDataRetriever::PublicAPI::OPEN_WEATHER_MAP, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                       PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, owmApiKey, 2);
  weatherRetriever.addPublicApiService(WeatherDataRetriever::PublicAPI::TOMORROW_IO, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                       PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, tomorrowIoApiKey, 3);
}

void Fusion::cleanup(void)
{
  // Clean up any resources used by the weather retriever
  weatherRetriever.removePublicApiService(WeatherDataRetriever::PublicAPI::NOAA);
  weatherRetriever.removePublicApiService(WeatherDataRetriever::PublicAPI::OPEN_WEATHER_MAP);
  weatherRetriever.removePublicApiService(WeatherDataRetriever::PublicAPI::TOMORROW_IO);
}

bool Fusion::validateFeasibility(const GunshotReport* validationPacket, const GunshotReport* packet)
{
  return (std::abs(packet->timestamp - validationPacket->timestamp) <=
          CivicAlert::FUSION_MAX_POSSIBLE_TIME_DIFFERENCE_SECONDS) &&  // TODO: TIME DIFFERENCE DOESN'T MAKE AS MUCH SENSE IF WE ARE ALLOWING MULTIPLE SHOTS IN SAME BUNDLE
         (GeoMath::calculateDistance3D(validationPacket->lat, validationPacket->lon, validationPacket->ht, packet->lat, packet->lon, packet->ht) <
          CivicAlert::FUSION_MAX_POSSIBLE_DISTANCE_METERS);  // TODO: 2.0*MAX_DISTANCE TO MAKE DISTANCE A WORST-CASE RADIUS FROM SHOOTER, NOT FROM SENSOR
}

// TODO: KEEP TRACK OF PREVIOUS LOCALIZATIONS IN SAME AREA TO INCREASE LIKELIHOOD THAT SHOOTING HAPPENED AT SAME SPOT, DECAY OVER TIME

IncidentMessage Fusion::performFusion(const std::vector<std::shared_ptr<GunshotReport>>& packetBundle)
{
  // Transform the sensor positions into an ENU coordinate frame
  logger.log(Logger::INFO, "Performing fusion on %zu packets\n", packetBundle.size());
  auto latCentroid = 0.0f, lonCentroid = 0.0f, heightCentroid = 0.0f;
  for (const auto& report : packetBundle)
  {
    latCentroid += report->lat;
    lonCentroid += report->lon;
    heightCentroid += report->ht;
  }
  latCentroid /= packetBundle.size();
  lonCentroid /= packetBundle.size();
  heightCentroid /= packetBundle.size();
  logger.log(Logger::DEBUG, "Centroid of Sensor Positions (LLH): <%.8f, %.8f, %.8f>\n", latCentroid, lonCentroid, heightCentroid);
  logger.log(Logger::DEBUG, "Sensor Positions (LLH) / Sensor Positions (ENU)\n");
  for (size_t i = 0; i < packetBundle.size(); ++i)
  {
    const auto& report(packetBundle.at(i));
    std::tie(report->east, report->north, report->up) = GeoMath::convertEcefToEnu(latCentroid, lonCentroid, heightCentroid, report->x, report->y, report->z);
    logger.log(Logger::DEBUG, "\t<%.8f, %.8f, %.8f> / <%.6f, %.6f, %.6f>\n", report->lat, report->lon, report->ht, report->east, report->north, report->up);
  }

  // Fetch the current speed of sound at the location of the reporting sensors
  const auto weatherData = weatherRetriever.getCurrentWeatherData(latCentroid, lonCentroid);
  const auto localSpeedOfSound = Math::SPEED_OF_SOUND_IN_AIR_METERS_PER_SECOND(weatherData.isValid ? weatherData.temperature : 20.0f);
  logger.log(Logger::DEBUG, "Speed of sound at event location = %f m/s\n", localSpeedOfSound);

  // Localize events and transform the ENU results back into a geodetic coordinate frame
  float latResult = 0.0f, lonResult = 0.0f, heightResult = 0.0f;
  auto result = localizeUsingHoughTransform(localSpeedOfSound, packetBundle);
  if (!result.eventTimes.empty())
  {
    const auto [xResult, yResult, zResult] = GeoMath::convertEnuToEcef(latCentroid, lonCentroid, heightCentroid, result.east, result.north, result.up);
    std::tie(result.lat, result.lon, result.ht) = GeoMath::convertEcefToLlh(xResult, yResult, zResult);
    logger.log(Logger::INFO, "Localization successful: Num shots = %zu, LLH pos = <%f, %f, %f>\n", result.eventTimes.size(), result.lat, result.lon, result.ht);
  }
  else
    logger.log(Logger::ERROR, "Unable to determine the location of the acoustic event\n");
  return result;
}

IncidentMessage Fusion::localizeUsingHoughTransform(float localSpeedOfSound, const std::vector<std::shared_ptr<GunshotReport>>& packetBundle)
{
  IncidentMessage result;
  /*const auto [minEast, maxEast, minNorth, maxNorth, minUp, maxUp] = std::make_tuple(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  for (auto east = minEast; east <= maxEast; east += CivicAlert::HOUGH_TRANSFORM_COARSE_RESOLUTION_METERS)
    for (auto north = minNorth; north <= maxNorth; north += CivicAlert::HOUGH_TRANSFORM_COARSE_RESOLUTION_METERS)
      for (auto up = minUp; up <= maxUp; up += CivicAlert::HOUGH_TRANSFORM_COARSE_RESOLUTION_METERS)
      {
      }*/
  return result;
}

// TODO: SOME NEW FUNCTION TO TRY AND DETECT FIREWORKS, MAYBE BASED ON LARGE NUMBER OF UNITS DETECTING SAME SHOT AT SIMILAR TIME, OR BASED ON ABOVE-UNIT AOAS (THIS SHOULD BE
// CALLED SOMEWHERE DURING THE "VALIDATION" PROCEDURE)
/*
IncidentMessage SensorFusion::localizeUsingHoughTransform(float localSpeedOfSound, const std::vector<std::shared_ptr<GunshotReport>>& packetBundle)
{
  // Set up all local components needed for the computation
  Logger::log(Logger::LoggingSeverity::DEBUG, "Beginning localization using the Hough Transform\n");
  bool wasSuccessful = false;
  std::vector<std::tuple<float, float, float, float, std::bitset<2 * Constants::MAX_GSL_REPORTS_PER_FUSION>>> peaks;
  float eastResult = 0.0f, northResult = 0.0f, upResult = 0.0f, error = 10000000.0f;
  float minEast = 1000000.0f, maxEast = -1000000.0f, minNorth = 1000000.0f, maxNorth = -1000000.0f;
  float eventTimeGpsSeconds = 0.0, minTime = std::numeric_limits<float>::max(), maxTime = std::numeric_limits<float>::lowest();
  for (const auto& gslReport : gslReports)
  {
    // Determine the minimum and maximum boundaries of the search space
    if (gslReport->east > maxEast) maxEast = gslReport->east;
    if (gslReport->east < minEast) minEast = gslReport->east;
    if (gslReport->north > maxNorth) maxNorth = gslReport->north;
    if (gslReport->north < minNorth) minNorth = gslReport->north;
    if (gslReport->eventTimeValid)
    {
      if (gslReport->relativeTimeOfArrival > maxTime) maxTime = gslReport->relativeTimeOfArrival;
      if (gslReport->relativeTimeOfArrival < minTime) minTime = gslReport->relativeTimeOfArrival;
    }
  }
  if (maxTime < 0.0f) minTime = 0.0f;

  // Carry out the Hough Transform calculations on increasingly coarse search spaces
  HoughTransform houghTransform;
  float resolutionMeters = 0.0f;
  for (int i = 0; (i < 3) && peaks.empty(); ++i)
  {
    resolutionMeters = (i == 0) ? Constants::HOUGH_TRANSFORM_FINE_RESOLUTION_METERS
                                : ((i == 1) ? Constants::HOUGH_TRANSFORM_MEDIUM_RESOLUTION_METERS : Constants::HOUGH_TRANSFORM_COARSE_RESOLUTION_METERS);
    houghTransform.reset2D(minEast - maxFusionDistance, maxEast + maxFusionDistance, minNorth - maxFusionDistance, maxNorth + maxFusionDistance,
                           minTime - (maxFusionDistance / localSpeedOfSound), minTime, resolutionMeters, localSpeedOfSound);
    for (const auto& toaIndex : toaIndices)
      houghTransform.transformTimeOfArrival(gslReports[toaIndex]->relativeTimeOfArrival, gslReports[toaIndex]->east, gslReports[toaIndex]->north, gslReports[toaIndex]->up,
                                            toaIndex);
    for (const auto& toaAoaIndex : toaAoaIndices)
    {
      houghTransform.transformTimeOfArrival(gslReports[toaAoaIndex]->relativeTimeOfArrival, gslReports[toaAoaIndex]->east, gslReports[toaAoaIndex]->north,
                                            gslReports[toaAoaIndex]->up, toaAoaIndex);
      houghTransform.transformAngleOfArrival(gslReports[toaAoaIndex]->relativeTimeOfArrival, gslReports[toaAoaIndex]->angleOfArrival[0],
gslReports[toaAoaIndex]->angleOfArrival[1], gslReports[toaAoaIndex]->angleOfArrival[2], gslReports[toaAoaIndex]->east, gslReports[toaAoaIndex]->north,
gslReports[toaAoaIndex]->up, toaAoaIndex + Constants::MAX_GSL_REPORTS_PER_FUSION);
    }
    for (const auto& aoaIndex : aoaIndices)
      houghTransform.transformAngleOfArrival(gslReports[aoaIndex]->angleOfArrival[0], gslReports[aoaIndex]->angleOfArrival[1], gslReports[aoaIndex]->angleOfArrival[2],
                                             gslReports[aoaIndex]->east, gslReports[aoaIndex]->north, gslReports[aoaIndex]->up, aoaIndex +
Constants::MAX_GSL_REPORTS_PER_FUSION, maxFusionDistance); for (const auto& peak : houghTransform.locateMaximumPeaks(Constants::MIN_NUM_HOUGH_PEAKS_FOR_FUSION, gslReports))
    {
      std::tuple<float, float, float, double> xyzt(
          houghTransform.convertBinsToXYZT(std::get<0>(peak.first), std::get<1>(peak.first), std::get<2>(peak.first), std::get<3>(peak.first), true));
      peaks.emplace_back(std::get<0>(xyzt), std::get<1>(xyzt), std::get<2>(xyzt), std::get<3>(xyzt), peak.second);
    }

    // Ensure that one of the maximum peaks includes the primary GSL report
    for (auto peakIt = std::begin(peaks); peakIt != std::end(peaks);)
      peakIt = (!std::get<4>(*peakIt).test(0) && !std::get<4>(*peakIt).test(Constants::MAX_GSL_REPORTS_PER_FUSION)) ? peaks.erase(peakIt) : (peakIt + 1);
    Logger::log(Logger::LoggingSeverity::DEBUG, "%d %s Hough Transform peak(s) found", peaks.size(), (i == 0) ? "Fine" : ((i == 1) ? "Medium" : "Coarse"));
  }
  if (peaks.empty()) return std::make_tuple(wasSuccessful, eventTimeGpsSeconds, eastResult, northResult, upResult, error);

  // Combine peaks with the same ENU coordinates and support bitmaps
  for (size_t i = 0; i < peaks.size(); ++i)
  {
    int numAccumulatedTimes = 1;
    double timeValueAccumulation = std::get<3>(peaks[i]);
    for (auto peak2 = std::begin(peaks) + i + 1; peak2 != std::end(peaks);)
      if ((*reinterpret_cast<uint64_t*>(&std::get<0>(peaks[i])) == (*reinterpret_cast<const uint64_t*>(&std::get<0>(*peak2)))) &&
          (*reinterpret_cast<uint64_t*>(&std::get<1>(peaks[i])) == (*reinterpret_cast<const uint64_t*>(&std::get<1>(*peak2)))) &&
          (*reinterpret_cast<uint64_t*>(&std::get<2>(peaks[i])) == (*reinterpret_cast<const uint64_t*>(&std::get<2>(*peak2)))) && (std::get<4>(peaks[i]) ==
std::get<4>(*peak2)))
      {
        timeValueAccumulation += std::get<3>(*peak2);
        ++numAccumulatedTimes;
        peak2 = peaks.erase(peak2);
      }
      else
        ++peak2;
    std::get<3>(peaks[i]) = (timeValueAccumulation / numAccumulatedTimes);
  }
  Logger::log(Logger::LoggingSeverity::DEBUG, "Combining peaks at similar locations: %d remaining", peaks.size());

  // Loop through all valid peaks, refining each solution
  size_t maxNumPossibleSupporters = gslReports.size();
  std::bitset<2 * Constants::MAX_GSL_REPORTS_PER_FUSION> bestPeak;
  std::list<std::bitset<2 * Constants::MAX_GSL_REPORTS_PER_FUSION>> peakPermutations;
  float maxPositionError = 0.5f * resolutionMeters, maxTimeError = resolutionMeters / localSpeedOfSound;
  for (size_t i = 0; i < peaks.size(); ++i)
  {
    // Get a list of all valid peak permutations which do not include GSL reports from the same Node ID
    peakPermutations.clear();
    const auto& peak(peaks[i]);
    generateValidPeakPermutations(peakPermutations, gslReports, std::get<4>(peak));

    // Determine which peak permutation provides the best result
    for (const auto& peakPermutation : peakPermutations)
    {
      // Clear the existing measurement indices
      toaAoaIndices.clear();
      toaIndices.clear();
      aoaIndices.clear();

      // Determine the indices of the supporters used in this peak
      Logger::log(Logger::LoggingSeverity::DEBUG, "Peak found at <%.6f, %.6f, %.6f, %.6f> with %d supporters", std::get<0>(peak), std::get<1>(peak), std::get<2>(peak),
                  std::get<3>(peak), peakPermutation.count());
      for (size_t i = 0; i < maxNumPossibleSupporters; ++i)
        if (peakPermutation.test(i))
        {
          if (peakPermutation.test(i + Constants::MAX_GSL_REPORTS_PER_FUSION))
            toaAoaIndices.emplace_back(i);
          else
            toaIndices.emplace_back(i);
        }
        else if (peakPermutation.test(i + Constants::MAX_GSL_REPORTS_PER_FUSION))
          aoaIndices.emplace_back(i);

      // Perform a non-linear least squares error minimization algorithm
      std::tuple<bool, double, float, float, float, float> result(localizeUsingErrorMinimization(localSpeedOfSound, gslReports, toaIndices, toaAoaIndices, aoaIndices,
                                                                                                 std::get<0>(peak), std::get<1>(peak), std::get<2>(peak), std::get<3>(peak),
                                                                                                 maxPositionError, maxTimeError));
      if (std::get<5>(result) < error)
      {
        std::tie(wasSuccessful, eventTimeGpsSeconds, eastResult, northResult, upResult, error) = result;
        bestPeak = peakPermutation;
        if (maxTime < 0.0f) eventTimeGpsSeconds = -10000000000.0;
      }
    }
  }
  Logger::log(Logger::LoggingSeverity::DEBUG, "Best refined location found at: ENU pos = <%.6f, %.6f, %.6f>, time = %.6f, error = %g", eastResult, northResult, upResult,
              eventTimeGpsSeconds, error);

  // Refine the best solution using a finer-grained multilateration search
  if (bestPeak.count())
  {
    // Determine the indices of the supporters used in this peak
    toaAoaIndices.clear();
    toaIndices.clear();
    aoaIndices.clear();
    for (size_t i = 0; i < maxNumPossibleSupporters; ++i)
      if (bestPeak.test(i))
      {
        if (bestPeak.test(i + Constants::MAX_GSL_REPORTS_PER_FUSION))
          toaAoaIndices.emplace_back(i);
        else
          toaIndices.emplace_back(i);
      }
      else if (bestPeak.test(i + Constants::MAX_GSL_REPORTS_PER_FUSION))
        aoaIndices.emplace_back(i);

    // Perform a fine-grained multilateration algorithm
    std::tuple<bool, double, float, float, float, float> refinedResults(refineUsingErrorMinimization(
        localSpeedOfSound, gslReports, toaIndices, toaAoaIndices, aoaIndices, eastResult, northResult, upResult, eventTimeGpsSeconds, maxPositionError, maxTimeError));
    if (std::get<0>(refinedResults)) std::tie(wasSuccessful, eventTimeGpsSeconds, eastResult, northResult, upResult, error) = refinedResults;
    if (maxTime < 0.0f) eventTimeGpsSeconds = -10000000000.0;

    // Update the "used" flag for all GSL reports that took part in this localization
    for (size_t i = 0; i < maxNumPossibleSupporters; ++i)
      if (bestPeak.test(i) || bestPeak.test(i + Constants::MAX_GSL_REPORTS_PER_FUSION)) gslReports[i]->usedInFusion.store(true);
  }
  return std::make_tuple(wasSuccessful, eventTimeGpsSeconds, eastResult, northResult, upResult, error);
}

std::tuple<bool, double, float, float, float, float> SensorFusion::localizeUsingErrorMinimization(
    float localSpeedOfSound, const std::vector<std::shared_ptr<GslDataReport>>& gslReports, const std::vector<int>& toaIndices, const std::vector<int>& toaAoaIndices,
    const std::vector<int>& aoaIndices, float initialGuessX, float initialGuessY, float initialGuessZ, float initialGuessT, float maxPositionError, float maxTimeError)
{
  // Ensure that there is enough data to carry out the optimization
  bool wasSuccessful = false;
  if ((toaIndices.size() + (2 * toaAoaIndices.size()) + aoaIndices.size()) < 4) return std::make_tuple(wasSuccessful, 0.0, 0.0f, 0.0f, 0.0f, 10000000.0f);

  // Search for an initial estimate of the "up" component since the Hough transform was 2D
  double upGuess, upError = 10000000.0;
  wasSuccessful = Optimizer<double>::minimize(Optimizer<double>::OptimizerType::NELDER_MEAD, &upGuess, upError,
                                              std::bind(&SensorFusion::errorFunctionHeight, gslReports, toaIndices, toaAoaIndices, aoaIndices, localSpeedOfSound, initialGuessX,
                                                        initialGuessY, initialGuessT, std::placeholders::_1, std::placeholders::_2),
                                              1, toaIndices.size() + (2 * toaAoaIndices.size()) + aoaIndices.size(), std::vector<double>{initialGuessZ}.data(), 0.1, 10000,
                                              std::vector<double>{maxFusionDistance}.data()) &&
                  (upError < 200.0);
  if (!wasSuccessful) upGuess = initialGuessZ;

  // Set up all local variables and carry out the non-linear least squares optimization
  double resultError = 10000000.0, resultLocation[4], startingGuessArray[]{initialGuessX, initialGuessY, upGuess, initialGuessT};
  wasSuccessful = Optimizer<double>::minimize(
                      Optimizer<double>::OptimizerType::LEVENBERG_MARQUARDT, resultLocation, resultError,
                      std::bind(&SensorFusion::errorFunction, gslReports, toaIndices, toaAoaIndices, aoaIndices, localSpeedOfSound, std::placeholders::_1,
std::placeholders::_2), 4, toaIndices.size() + (2 * toaAoaIndices.size()) + aoaIndices.size(), startingGuessArray, 0.1, 1000, std::vector<double>{maxPositionError,
maxPositionError, maxPositionError, maxTimeError}.data()) && (resultError < 100.0);

  // Return all results and whether or not the optimization was successful
  if (wasSuccessful)
    Logger::log(Logger::LoggingSeverity::DEBUG, "Refining the position estimate: <%.6f, %.6f, %.6f, %.6f>, Error residual = %g", resultLocation[0], resultLocation[1],
                resultLocation[2], resultLocation[3], resultError);
  return std::make_tuple(wasSuccessful, resultLocation[3], static_cast<float>(resultLocation[0]), static_cast<float>(resultLocation[1]), static_cast<float>(resultLocation[2]),
                         static_cast<float>(resultError));
}

std::tuple<bool, double, float, float, float, float> SensorFusion::refineUsingErrorMinimization(
    float localSpeedOfSound, const std::vector<std::shared_ptr<GslDataReport>>& gslReports, const std::vector<int>& toaIndices, const std::vector<int>& toaAoaIndices,
    const std::vector<int>& aoaIndices, float initialGuessX, float initialGuessY, float initialGuessZ, float initialGuessT, float maxPositionError, float maxTimeError)
{
  // Ensure that there is enough data to carry out the optimization
  bool wasSuccessful = false;
  if ((toaIndices.size() + (2 * toaAoaIndices.size()) + aoaIndices.size()) < 4) return std::make_tuple(wasSuccessful, 0.0, 0.0f, 0.0f, 0.0f, 10000000.0f);

  // Set up all local variables and carry out the non-linear least squares optimization
  double resultError = 10000000.0, resultLocation[4], startingGuessArray[]{initialGuessX, initialGuessY, initialGuessZ, initialGuessT};
  wasSuccessful = Optimizer<double>::minimize(
                      Optimizer<double>::OptimizerType::NELDER_MEAD, resultLocation, resultError,
                      std::bind(&SensorFusion::errorFunction, gslReports, toaIndices, toaAoaIndices, aoaIndices, localSpeedOfSound, std::placeholders::_1,
std::placeholders::_2), 4, toaIndices.size() + (2 * toaAoaIndices.size()) + aoaIndices.size(), startingGuessArray, 0.01, 50000, std::vector<double>{maxPositionError,
maxPositionError, maxPositionError, maxTimeError}.data()) && (resultError < 100.0);

  // Return all results and whether or not the optimization was successful
  Logger::log(Logger::LoggingSeverity::DEBUG, "Fine-tuning the position estimate: <%.6f, %.6f, %.6f, %.6f>, Error residual = %g", resultLocation[0], resultLocation[1],
              resultLocation[2], resultLocation[3], resultError);
  return std::make_tuple(wasSuccessful, resultLocation[3], static_cast<float>(resultLocation[0]), static_cast<float>(resultLocation[1]), static_cast<float>(resultLocation[2]),
                         static_cast<float>(resultError));
}

void SensorFusion::errorFunction(const std::vector<std::shared_ptr<GslDataReport>>& gslReports, const std::vector<int>& toaIndices, const std::vector<int>& toaAoaIndices,
                                 const std::vector<int>& aoaIndices, float speedOfSound, double guess[], double errors[])
{
  // Accumulate error based on the angle of arrival measurements
  int errorIndex = 0;
  double east = guess[0], north = guess[1], up = guess[2], t = guess[3], errorScaler = 1.0 / (toaIndices.size() + (2 * toaAoaIndices.size()) + aoaIndices.size());
  for (const auto& aoaIndex : aoaIndices)
  {
    const auto& gslReport(gslReports[aoaIndex]);
    double range =
        std::sqrt((gslReport->east - east) * (gslReport->east - east) + (gslReport->north - north) * (gslReport->north - north) + (gslReport->up - up) * (gslReport->up - up));
    double rangeInv = 1.0 / range;
    double eastError = range * (((east - gslReport->east) * rangeInv) - gslReport->angleOfArrival[0]);
    double northError = range * (((north - gslReport->north) * rangeInv) - gslReport->angleOfArrival[1]);
    double upError = range * (((up - gslReport->up) * rangeInv) - gslReport->angleOfArrival[2]);
    errors[errorIndex++] = std::sqrt(errorScaler * std::sqrt(eastError * eastError + northError * northError + upError * upError));
  }

  // Accumulate error based on the combined TOA/AOA measurements
  for (const auto& toaAoaIndex : toaAoaIndices)
  {
    const auto& gslReport(gslReports[toaAoaIndex]);
    double eastError = (speedOfSound * gslReport->angleOfArrival[0] * (gslReport->relativeTimeOfArrival - t)) - (east - gslReport->east);
    double northError = (speedOfSound * gslReport->angleOfArrival[1] * (gslReport->relativeTimeOfArrival - t)) - (north - gslReport->north);
    double upError = (speedOfSound * gslReport->angleOfArrival[2] * (gslReport->relativeTimeOfArrival - t)) - (up - gslReport->up);
    errors[errorIndex++] = std::sqrt(errorScaler * std::sqrt(eastError * eastError + northError * northError + upError * upError));
    errors[errorIndex++] = std::sqrt(errorScaler * std::abs(std::sqrt((gslReport->east - east) * (gslReport->east - east) +
                                                                      (gslReport->north - north) * (gslReport->north - north) + (gslReport->up - up) * (gslReport->up - up)) -
                                                            (speedOfSound * (gslReport->relativeTimeOfArrival - t))));
  }

  // Accumulate error based on the time of arrival measurements
  for (const auto& toaIndex : toaIndices)
  {
    const auto& gslReport(gslReports[toaIndex]);
    errors[errorIndex++] = std::sqrt(errorScaler * std::abs(std::sqrt((gslReport->east - east) * (gslReport->east - east) +
                                                                      (gslReport->north - north) * (gslReport->north - north) + (gslReport->up - up) * (gslReport->up - up)) -
                                                            (speedOfSound * (gslReport->relativeTimeOfArrival - t))));
  }
}

void SensorFusion::errorFunctionHeight(const std::vector<std::shared_ptr<GslDataReport>>& gslReports, const std::vector<int>& toaIndices, const std::vector<int>& toaAoaIndices,
                                       const std::vector<int>& aoaIndices, float speedOfSound, double east, double north, double t, double guess[], double errors[])
{
  // Simply forward this calculation to the general error function with the proper variables bound
  errorFunction(gslReports, toaIndices, toaAoaIndices, aoaIndices, speedOfSound, std::vector<double>{east, north, guess[0], t}.data(), errors);
}

void SensorFusion::generateValidPeakPermutations(std::list<std::bitset<2 * Constants::MAX_GSL_REPORTS_PER_FUSION>>& peakPermutations,
                                                 const std::vector<std::shared_ptr<GslDataReport>>& gslReports,
                                                 const std::bitset<2 * Constants::MAX_GSL_REPORTS_PER_FUSION>& peakData, size_t startingIndex,
                                                 std::bitset<2 * Constants::MAX_GSL_REPORTS_PER_FUSION>* currentPermutation, std::unordered_map<std::string, size_t>
alreadyUsedSet)
{
  // Fetch the current working permutation from the permutation vector
  bool wasFirstCall = false;
  if (!currentPermutation)
  {
    wasFirstCall = true;
    peakPermutations.emplace_back();
    currentPermutation = &peakPermutations.back();
  }

  // Iterate through all GSL reports looking for valid permutations with non-overlapping Node IDs
  for (; startingIndex < gslReports.size(); ++startingIndex)
  {
    bool alreadyUsed = alreadyUsedSet.count(gslReports[startingIndex]->nodeID);
    if (!alreadyUsed && (peakData.test(startingIndex) || peakData.test(startingIndex + Constants::MAX_GSL_REPORTS_PER_FUSION)))
    {
      // Set the used flags for the parts of the GSL report used in this peak
      if (peakData.test(startingIndex)) currentPermutation->set(startingIndex);
      if (peakData.test(startingIndex + Constants::MAX_GSL_REPORTS_PER_FUSION)) currentPermutation->set(startingIndex + Constants::MAX_GSL_REPORTS_PER_FUSION);

      // Set the ID of the current GSL report to used and recursively search for more reports
      alreadyUsedSet.emplace(gslReports[startingIndex]->nodeID, startingIndex);
      generateValidPeakPermutations(peakPermutations, gslReports, peakData, startingIndex + 1, currentPermutation, alreadyUsedSet);
      break;
    }
    else if (alreadyUsed)
    {
      // Continue searching for valid permutations using the current peak information
      generateValidPeakPermutations(peakPermutations, gslReports, peakData, startingIndex + 1, currentPermutation, alreadyUsedSet);

      // Remove the peak information for the previous matching GSL report and add the current peak information
      size_t usedIndex = alreadyUsedSet.at(gslReports[startingIndex]->nodeID);
      peakPermutations.emplace_back(*currentPermutation);
      std::bitset<2 * Constants::MAX_GSL_REPORTS_PER_FUSION>* newPermutation = &peakPermutations.back();
      newPermutation->reset(usedIndex);
      newPermutation->reset(usedIndex + Constants::MAX_GSL_REPORTS_PER_FUSION);
      if (peakData.test(startingIndex)) newPermutation->set(startingIndex);
      if (peakData.test(startingIndex + Constants::MAX_GSL_REPORTS_PER_FUSION)) newPermutation->set(startingIndex + Constants::MAX_GSL_REPORTS_PER_FUSION);

      // Mark the ID of the current GSL report as "used" and recursively search for more usable reports
      size_t permutationAlreadyExistsCount = 0;
      for (const auto& permutation : peakPermutations)
        if (permutation == *newPermutation) ++permutationAlreadyExistsCount;
      if (permutationAlreadyExistsCount > 1)
        peakPermutations.pop_back();
      else
      {
        alreadyUsedSet.at(gslReports[startingIndex]->nodeID) = startingIndex;
        generateValidPeakPermutations(peakPermutations, gslReports, peakData, startingIndex + 1, newPermutation, alreadyUsedSet);
      }
      break;
    }
  }

  // Delete all peak permutations that have less support than the maximum peak permutation
  if (wasFirstCall)
  {
    size_t maxNumSupporters = 0;
    for (const auto& permutation : peakPermutations)
      if (permutation.count() > maxNumSupporters) maxNumSupporters = permutation.count();
    for (auto it = std::begin(peakPermutations); it != std::end(peakPermutations);)
      if (it->count() < maxNumSupporters)
        it = peakPermutations.erase(it);
      else
        ++it;
  }
}
*/

#include "GeoMath.h"
#include "MathUtilities.h"
#include "WeatherRetriever.h"

WeatherRetriever::WeatherRetriever(void) : weatherDataRetriever(), currentWeatherTable() {}
WeatherRetriever::~WeatherRetriever(void) {}

bool WeatherRetriever::addPublicApiService(WeatherDataRetriever::PublicAPI apiType, PublicWeatherAPI::TemperatureUnits unitsOfTemperature,
                                           PublicWeatherAPI::PressureUnits unitsOfPressure, PublicWeatherAPI::WindSpeedUnits unitsOfWindSpeed, const char* apiKey, int8_t priority)
{
  return weatherDataRetriever.addPublicApiService(apiType, unitsOfTemperature, unitsOfPressure, unitsOfWindSpeed, apiKey, priority);
}

void WeatherRetriever::removePublicApiService(WeatherDataRetriever::PublicAPI apiType) { weatherDataRetriever.removePublicApiService(apiType); }

WeatherData WeatherRetriever::getCurrentWeatherData(float latitude, float longitude)
{
  // Search for already existing weather data within 2.5km of the specified coordinates
  WeatherData weatherData;
  const time_t currentTime = time(nullptr);
  const float latRadians = latitude * Math::DEGREES_TO_RADIANS, lonRadians = longitude * Math::DEGREES_TO_RADIANS;
  for (auto it = std::cbegin(currentWeatherTable); it != std::cend(currentWeatherTable);)
  {
    // Clean up the weather table along the way, removing elements older than 30 minutes
    if ((currentTime - std::get<0>(*it)) > 1800)
    {
      it = currentWeatherTable.erase(it);
      continue;
    }
    else if (GeoMath::estimateDistance2D(latRadians, lonRadians, std::get<1>(*it), std::get<2>(*it)) < 2500.0f)
      weatherData = std::get<3>(*it);
    ++it;
  }

  // Retrieve data from a public weather API if no existing data was found
  if (!weatherData.isValid && (weatherData = weatherDataRetriever.getCurrentWeatherData(latitude, longitude)).isValid)
    currentWeatherTable.emplace_back(currentTime, latRadians, lonRadians, weatherData);
  return weatherData;
}

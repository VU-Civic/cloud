#ifndef __WEATHER_RETRIEVER_HEADER_H__
#define __WEATHER_RETRIEVER_HEADER_H__

#include <ctime>
#include <tuple>
#include <vector>
#include "WeatherDataRetriever.h"

class WeatherRetriever final
{
public:

  // Constructor/destructor
  WeatherRetriever(void);
  ~WeatherRetriever(void);

  // Weather API service management functionality
  bool addPublicApiService(WeatherDataRetriever::PublicAPI apiType, PublicWeatherAPI::TemperatureUnits unitsOfTemperature, PublicWeatherAPI::PressureUnits unitsOfPressure,
                           PublicWeatherAPI::WindSpeedUnits unitsOfWindSpeed, const char* apiKey = nullptr, int8_t priority = -1);
  void removePublicApiService(WeatherDataRetriever::PublicAPI apiType);
  WeatherData getCurrentWeatherData(float latitude, float longitude);

private:

  // Private member variables
  WeatherDataRetriever weatherDataRetriever;
  std::vector<std::tuple<time_t, float, float, WeatherData>> currentWeatherTable;
};

#endif  // #ifndef __WEATHER_RETRIEVER_HEADER_H__

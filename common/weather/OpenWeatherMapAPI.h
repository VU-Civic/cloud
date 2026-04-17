#ifndef __OPEN_WEATHER_MAP_API_HEADER_H__
#define __OPEN_WEATHER_MAP_API_HEADER_H__

#include <string>
#include "WeatherDataRetriever.h"

class OpenWeatherMapAPI final : public PublicWeatherAPI
{
public:

  // Constructor/destructor
  OpenWeatherMapAPI(const char* openWeatherMapApiKey, TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed);
  ~OpenWeatherMapAPI(void);

  // Weather data retrieval functions
  WeatherData getCurrentWeatherData(void* curl, float latitude, float longitude) const override;

private:

  // Private member variables
  const std::string apiKey;
  std::string weatherUrlBegin, weatherUrlEnd;
};

#endif  // #ifndef __OPEN_WEATHER_MAP_API_HEADER_H__
